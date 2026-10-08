#!/usr/bin/env python3
"""
Headless QEMU Smoke Test Runner for Smart-Appliance-Edge-DevOps
==============================================================
Monitors the virtual UART stream from qemu-system-arm and asserts
that the compiled firmware boots and reaches its idle control loop.
"""

import subprocess
import time
import sys
import os
import select

# --- Configuration ---
TIMEOUT_SECONDS = 6
BINARY_PATH = "build/zephyr/zephyr.elf"

QEMU_CMD = [
    "qemu-system-arm",
    "-machine", "lm3s6965evb",
    "-cpu", "cortex-m3",
    "-nographic",
    "-kernel", BINARY_PATH
]

# Strings that prove the firmware successfully booted and entered RTOS scheduler
REQUIRED_PASS_MARKERS = [
    "Booting on board",
    "Motor Supervisory Controller is Idle..."
]

# Strings that indicate a fatal firmware crash or RTOS assertion
FATAL_FAIL_MARKERS = [
    "Kernel Panic",
    "ASSERTION FAIL",
    "Hard Fault",
    "Faulting instruction",
    "USAGE FAULT"
]


def run_qemu_smoke_test():
    # 1. Sanity check: Ensure the ELF binary was compiled before launching
    if not os.path.isfile(BINARY_PATH):
        print(f"[-] ERROR: Target firmware binary not found at '{BINARY_PATH}'")
        print("[-] HINT: Run 'west build -p always -b qemu_cortex_m3 firmware' first.")
        return 1

    print("=" * 60)
    print("[*] Starting Headless QEMU Smoke Test...")
    print(f"[*] Target Binary : {BINARY_PATH}")
    print(f"[*] Command       : {' '.join(QEMU_CMD)}")
    print(f"[*] Timeout Limit : {TIMEOUT_SECONDS}s")
    print("=" * 60)

    # 2. Launch QEMU as a supervised child process
    proc = subprocess.Popen(
        QEMU_CMD,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    start_time = time.time()
    seen_markers = set()
    test_passed = False
    failure_reason = ""

    try:
        while time.time() - start_time < TIMEOUT_SECONDS:
            # Check if QEMU died unexpectedly early
            if proc.poll() is not None:
                failure_reason = f"QEMU process exited unexpectedly with code {proc.returncode}"
                break

            # Non-blocking check for available output (wait up to 0.5s)
            readable, _, _ = select.select([proc.stdout], [], [], 0.5)
            if readable:
                line = proc.stdout.readline()
                if not line:
                    break

                # Print live output so CI logs show real-time progress
                sys.stdout.write(f"  [QEMU UART] {line}")
                sys.stdout.flush()

                # Check for fatal crash signatures
                for fail_marker in FATAL_FAIL_MARKERS:
                    if fail_marker in line:
                        failure_reason = f"Detected fatal firmware crash: '{fail_marker}'"
                        break
                if failure_reason:
                    break

                # Check for required pass markers
                for pass_marker in REQUIRED_PASS_MARKERS:
                    if pass_marker in line:
                        seen_markers.add(pass_marker)

                # If all required boot markers are matched, pass the test
                if len(seen_markers) == len(REQUIRED_PASS_MARKERS):
                    test_passed = True
                    break

        if not test_passed and not failure_reason:
            missing = [m for m in REQUIRED_PASS_MARKERS if m not in seen_markers]
            failure_reason = f"Timed out after {TIMEOUT_SECONDS}s waiting for markers: {missing}"

    except Exception as e:
        failure_reason = f"Unexpected test execution error: {e}"

    finally:
        # 3. Clean Process Termination (Never leave QEMU running in background)
        print("\n[*] Terminating virtual microcontroller (QEMU)...")
        try:
            proc.terminate()
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            print("[!] QEMU did not shut down gracefully. Forcing SIGKILL...")
            proc.kill()
            proc.wait()

    # 4. Final Evaluation & Exit Code
    print("=" * 60)
    if test_passed:
        print("[+] TEST PASSED: Firmware booted cleanly into operational state!")
        print("=" * 60)
        return 0
    else:
        print(f"[-] TEST FAILED: {failure_reason}")
        print("=" * 60)
        return 1


if __name__ == "__main__":
    sys.exit(run_qemu_smoke_test())