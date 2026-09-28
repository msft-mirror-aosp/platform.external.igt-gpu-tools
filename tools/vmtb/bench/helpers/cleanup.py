# SPDX-License-Identifier: MIT
# Copyright © 2026 Intel Corporation

import logging
import subprocess
import time
from pathlib import Path

logger = logging.getLogger('Cleanup')


class EnvironmentCleaner:
    """Pre-test environment cleanup utilities.

    Ensures that leftovers from crashed, failed, or timed-out tests
    are removed before the next test setup phase begins.
    """

    QEMU_PROCESS_NAME = 'qemu-system-x86_64'
    SOCKET_PATTERNS = ['qga*.sock', 'mon*.sock']
    QEMU_IMAGE_PATTERN = 'vm*_*_image.qcow2'

    def __init__(self, work_dir: str = '.', remove_qemu_images: bool = False) -> None:
        self.work_dir = Path(work_dir)
        self.remove_qemu_images = remove_qemu_images

    def cleanup(self) -> None:
        """Perform full pre-test environment cleanup.
        Handles:
        - Terminating orphaned QEMU processes
        - Removing stale QEMU sockets (Guest Agent, QMP monitor)
        - Removing leftover QEMU qcow2 images (optionally)
        """
        logger.debug("Pre-test environment cleanup")
        self.terminate_orphaned_qemu_processes()
        self.remove_stale_sockets()
        if self.remove_qemu_images:
            self.remove_leftover_qemu_images()

    def terminate_orphaned_qemu_processes(self) -> None:
        """Find and terminate any leftover QEMU processes from previous test runs.
           Try SIGTERM first, then SIGKILL if necessary."""
        for signal in ('TERM', 'KILL'):
            pids = self._find_qemu_pids()
            if not pids:
                return
            for pid_str in pids:
                logger.debug("Terminating (SIG%s) orphaned QEMU process (PID: %s)", signal, pid_str)
                try:
                    subprocess.run(['kill', f'-{signal}', pid_str], check=False)
                except Exception as exc:
                    logger.warning("Failed to %s QEMU PID %s: %s", signal, pid_str, exc)
            time.sleep(1)  # Give the processes a moment to terminate

    def _find_qemu_pids(self) -> list[str] | None:
        """Return list of QEMU PIDs, or None if none found or pgrep unavailable."""
        try:
            result = subprocess.run(
                ['pgrep', '-f', self.QEMU_PROCESS_NAME],
                capture_output=True, text=True, check=False)
        except FileNotFoundError:
            logger.warning("pgrep not available, skipping orphan QEMU check")
            return None

        if result.returncode != 0:
            return None

        return [pid for pid in result.stdout.strip().split('\n') if pid] or None

    def remove_stale_sockets(self) -> None:
        """Remove leftover QEMU Guest Agent and QMP monitor sockets."""
        for pattern in self.SOCKET_PATTERNS:
            for sock_path in Path('/tmp').glob(pattern):
                try:
                    if not sock_path.is_socket():
                        continue
                    logger.debug("Removing stale VMTB/QEMU socket: %s", sock_path)
                    sock_path.unlink()
                except OSError as exc:
                    logger.warning("Failed to remove stale socket %s: %s", sock_path, exc)

    def remove_leftover_qemu_images(self) -> None:
        """Remove leftover QEMU qcow2 images from previous test runs."""
        for image_path in self.work_dir.glob(self.QEMU_IMAGE_PATTERN):
            try:
                if not image_path.is_file():
                    continue
                logger.debug("Removing leftover VMTB/QEMU guest image: %s", image_path)
                image_path.unlink()
            except OSError as exc:
                logger.warning("Failed to remove leftover image %s: %s", image_path, exc)
