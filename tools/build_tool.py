from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Sequence, Union

# NOTE: 下面两个默认路径来自旧 bat（Build.bat / GenProject.bat）。
# 如果你的 VS 安装路径或 MSVC 版本不同，请修改这里。
DEFAULT_VSVARSALL_PATH = r"E:\Programs\VisualStudio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
DEFAULT_ASAN_DLL_PATH = (
    r"E:\Programs\VisualStudio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
    r"\clang_rt.asan_dynamic-x86_64.dll"
)

Cmd = Union[str, Sequence[str]]


@dataclass(frozen=True)
class ToolPaths:
    vsvarsall_path: Path
    asan_dll_path: Path


@dataclass(frozen=True)
class BuildSettings:
    arch: str = "x64"
    build_type: str = "Debug"
    use_asan: bool = False


@dataclass(frozen=True)
class ProjectPaths:
    root: Path
    build_dir: Path
    ninja_dir: Path
    project_dir: Path

    @staticmethod
    def from_root(root: Path) -> "ProjectPaths":
        build_dir = root / "build"
        return ProjectPaths(
            root=root,
            build_dir=build_dir,
            ninja_dir=build_dir / "ninja",
            project_dir=build_dir / "project",
        )


class CommandRunner:
    def __init__(self) -> None:
        # 更接近“实时输出”的体验（尤其是被上层工具/IDE 捕获时）
        try:
            sys.stdout.reconfigure(line_buffering=True)
        except Exception:
            pass

    @staticmethod
    def _format_cmd_for_log(cmd: Cmd) -> str:
        if isinstance(cmd, str):
            return cmd
        try:
            return subprocess.list2cmdline(list(cmd))
        except Exception:
            return " ".join(map(str, cmd))

    def run(self, cmd: Cmd, *, cwd: Optional[Path] = None) -> int:
        """
        运行命令并实时输出 stdout/stderr（合并）。
        任何非 0 返回码由调用者决定是否立即终止。
        """
        cwd_str = str(cwd) if cwd is not None else None
        print(f"\n> {self._format_cmd_for_log(cmd)}")
        if cwd_str:
            print(f"  (cwd: {cwd_str})")

        # Windows 下某些工具输出不是 UTF-8，这里用默认编码 + 容错，避免崩溃。
        try:
            p = subprocess.Popen(
                cmd,
                cwd=cwd_str,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                errors="replace",
                bufsize=1,
            )
        except FileNotFoundError:
            # 例如 conan/cmake/ninja 未在 PATH 中
            print("\n[ERROR] 找不到可执行文件。请确认已安装并加入 PATH。")
            return 127
        except Exception as e:
            print(f"\n[ERROR] 启动子进程失败：{e}")
            return 1

        assert p.stdout is not None
        for line in p.stdout:
            print(line, end="")

        return p.wait()

    def run_in_vs_env(
        self,
        inner_cmd: Sequence[str],
        *,
        vsvarsall_path: Path,
        arch: str,
        cwd: Optional[Path] = None,
    ) -> int:
        """
        等价于 bat：
          call vcvarsall.bat x64
          <inner_cmd...>
        这里通过 cmd.exe 串联，避免在 Python 内解析环境变量。
        """
        # 使用“临时 .cmd 文件”来规避 Windows cmd.exe 引号/转义陷阱。
        inner_cmdline = subprocess.list2cmdline(list(inner_cmd))
        vs = str(vsvarsall_path)

        script = "\r\n".join(
            [
                "@echo off",
                f'call "{vs}" {arch}',
                "if errorlevel 1 exit /b %errorlevel%",
                inner_cmdline,
                "exit /b %errorlevel%",
                "",
            ]
        )

        tmp_path: Optional[Path] = None
        try:
            with tempfile.NamedTemporaryFile(
                mode="w",
                encoding="utf-8",
                newline="\r\n",
                suffix=".cmd",
                delete=False,
            ) as f:
                f.write(script)
                tmp_path = Path(f.name)

            return self.run(["cmd", "/d", "/s", "/c", str(tmp_path)], cwd=cwd)
        finally:
            if tmp_path is not None:
                try:
                    tmp_path.unlink(missing_ok=True)
                except Exception:
                    pass


class ExitBehavior:
    def __init__(self, *, success_wait_seconds: int = 5) -> None:
        self._success_wait_seconds = success_wait_seconds

    def finalize(self, rc: int, *, no_wait: bool) -> int:
        if rc == 0:
            self._wait_on_success(no_wait=no_wait)
        else:
            self._pause_on_error(no_wait=no_wait)
        return rc

    def _wait_on_success(self, *, no_wait: bool) -> None:
        if no_wait or self._success_wait_seconds <= 0:
            return
        print(f"\n[OK] 所有命令成功，将在 {self._success_wait_seconds} 秒后自动退出……")
        time.sleep(self._success_wait_seconds)

    @staticmethod
    def _pause_on_error(*, no_wait: bool) -> None:
        if no_wait:
            return
        if os.name == "nt":
            try:
                import msvcrt  # type: ignore

                print("按任意键退出……", end="", flush=True)
                msvcrt.getch()
                print()
                return
            except Exception:
                pass
        try:
            input("按回车退出……")
        except Exception:
            # stdin 不可用时，给用户一点时间看见错误输出
            print("[WARN] stdin 不可用，无法等待输入；5 秒后退出……")
            time.sleep(5)


class BuildTool:
    def __init__(
        self,
        *,
        paths: ProjectPaths,
        tools: ToolPaths,
        settings: BuildSettings,
        runner: CommandRunner,
    ) -> None:
        self._paths = paths
        self._tools = tools
        self._settings = settings
        self._runner = runner

    def validate(self) -> int:
        if not self._tools.vsvarsall_path.exists():
            print(f"[ERROR] 找不到 vcvarsall.bat：{self._tools.vsvarsall_path}")
            print("请修改 tools/build_tool.py 里的 DEFAULT_VSVARSALL_PATH 以匹配你的 VS 安装路径。")
            return 2
        return 0

    def generate(self) -> int:
        """
        等价于 GenProject.bat：
          - mkdir build
          - conan install .. --build=missing --settings build_type=Debug
          - rd /s /q build\\ninja
          - cmake -G "Visual Studio 17 2022" ...
          - (VS env) cmake -G "Ninja" ... -DCMAKE_BUILD_TYPE=Debug -DUSE_ASAN=OFF
        """
        self._paths.build_dir.mkdir(parents=True, exist_ok=True)

        rc = self._runner.run(
            [
                "conan",
                "install",
                "..",
                "--build=missing",
                "--settings",
                f"build_type={self._settings.build_type}",
            ],
            cwd=self._paths.build_dir,
        )
        if rc != 0:
            print(f"\n[ERROR] conan install 失败，返回码: {rc}")
            return rc

        if self._paths.ninja_dir.exists():
            shutil.rmtree(self._paths.ninja_dir, ignore_errors=True)

        rc = self._runner.run(
            [
                "cmake",
                "-G",
                "Visual Studio 17 2022",
                "-S",
                str(self._paths.root),
                "-B",
                str(self._paths.project_dir),
            ],
            cwd=self._paths.build_dir,
        )
        if rc != 0:
            print(f"\n[ERROR] 生成 Visual Studio 工程失败，返回码: {rc}")
            return rc

        rc = self._runner.run_in_vs_env(
            [
                "cmake",
                "-G",
                "Ninja",
                "-S",
                str(self._paths.root),
                "-B",
                str(self._paths.ninja_dir),
                f"-DCMAKE_BUILD_TYPE={self._settings.build_type}",
                f"-DUSE_ASAN={'ON' if self._settings.use_asan else 'OFF'}",
            ],
            vsvarsall_path=self._tools.vsvarsall_path,
            arch=self._settings.arch,
            cwd=self._paths.build_dir,
        )
        if rc != 0:
            print(f"\n[ERROR] 生成 Ninja 工程失败，返回码: {rc}")
            return rc

        return 0

    def build(self) -> int:
        """
        等价于 Build.bat：
          - (VS env) cd build\\ninja && ninja
          - 若存在 ASan DLL，则拷贝到 build\\ninja
        """
        if not self._paths.ninja_dir.exists():
            print(f"[ERROR] 找不到 Ninja 构建目录：{self._paths.ninja_dir}")
            print("请先运行 --action gen 或 --action all 生成工程。")
            return 2

        rc = self._runner.run_in_vs_env(
            ["ninja"],
            vsvarsall_path=self._tools.vsvarsall_path,
            arch=self._settings.arch,
            cwd=self._paths.ninja_dir,
        )
        if rc != 0:
            print(f"\n[ERROR] ninja 构建失败，返回码: {rc}")
            return rc

        # 旧脚本会在 USE_ASAN=OFF 时也尝试 copy 并打印警告，实际噪音大且没价值：
        # - 这里改为：存在则拷贝，不存在则静默跳过。
        if self._tools.asan_dll_path.exists():
            try:
                shutil.copy2(self._tools.asan_dll_path, self._paths.ninja_dir / self._tools.asan_dll_path.name)
                print(f"[OK] 已拷贝 ASan DLL：{self._tools.asan_dll_path.name}")
            except Exception as e:
                print(f"[WARN] 拷贝 ASan DLL 失败：{e}")

        return 0


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        prog="build_tool.py",
        description="Python 构建入口（替代 Build.bat / GenProject.bat）。",
    )
    p.add_argument(
        "-a",
        "--action",
        choices=("gen", "build", "all"),
        required=True,
        help="gen：生成工程；build：构建；all：先 gen 再 build。",
    )
    p.add_argument(
        "--no-wait",
        action="store_true",
        help="禁用退出等待/暂停（成功后不等待 5 秒；失败后不暂停）。适合 CI。",
    )
    return p.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)

    # tools/build_tool.py -> repo root
    project_root = Path(__file__).resolve().parents[1]
    runner = CommandRunner()

    tool = BuildTool(
        paths=ProjectPaths.from_root(project_root),
        tools=ToolPaths(vsvarsall_path=Path(DEFAULT_VSVARSALL_PATH), asan_dll_path=Path(DEFAULT_ASAN_DLL_PATH)),
        settings=BuildSettings(),
        runner=runner,
    )

    exit_behavior = ExitBehavior(success_wait_seconds=5)

    rc = tool.validate()
    if rc != 0:
        return exit_behavior.finalize(rc, no_wait=args.no_wait)

    if args.action in ("gen", "all"):
        rc = tool.generate()
        if rc != 0:
            return exit_behavior.finalize(rc, no_wait=args.no_wait)

    if args.action in ("build", "all"):
        rc = tool.build()
        if rc != 0:
            return exit_behavior.finalize(rc, no_wait=args.no_wait)

    return exit_behavior.finalize(0, no_wait=args.no_wait)


if __name__ == "__main__":
    raise SystemExit(main())

