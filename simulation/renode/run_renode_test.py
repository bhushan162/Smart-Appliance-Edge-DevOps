#!/usr/bin/env python3
import subprocess
import time
import sys
import os
import select

TIMEOUT_SECONDS = 35
BINARY_PATH = "build_stm32/zephyr/zephyr.elf"
RESC_SCRIPT = "simulation/renode/stm32_simulation.resc"

RENODE_CMD = [
    "renode",
    "--disable-xwt",
    "--console",
    "-p",
    "-e", f"s @{RESC_SCRIPT}; quit"
]

REQUIRED_PASS_MARKERS = [
    "Booting on board: nucleo_f401re",
    "Configured PB0-PB10 as Slider GPIO Inputs",
    "Configured PA5 as Motor Driver output",
    "Selected Speed 2 (SLOW MIX)",
    "Speed 2 [SLOW MIX] Stable at 250 RPM",
    "Selected Speed 6 (CREAMING HIGH)",
    "Speed 6 [CREAMING HIGH] Stable at 750 RPM",
    "Selected Speed 0 (OFF / IDLE)",
    "Motor stopped (0 RPM, 0% PWM)"
]

def run_test():
    if not os.path.isfile(BINARY_PATH):
        print(f"[-] ERROR: Binary not found at {BINARY_PATH}")
        return 1
    if not os.path.isfile(RESC_SCRIPT):
        print(f"[-] ERROR: Script not found at {RESC_SCRIPT}")
        return 1

    print("=" * 70)
    print("[*] Starting Headless Renode System Integration Test...")
    print(f"[*] Target Binary : {BINARY_PATH}")
    print(f"[*] Simulation    : {RESC_SCRIPT}")
    print(f"[*] Timeout Limit : {TIMEOUT_SECONDS}s")
    print("=" * 70)

    proc = subprocess.Popen(
        RENODE_CMD,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    start_time = time.time()
    seen = set()
    passed = False
    fail_msg = ""

    try:
        while time.time() - start_time < TIMEOUT_SECONDS:
            if proc.poll() is not None:
                for l in proc.stdout.read().splitlines():
                    sys.stdout.write(f"  [RENODE] {l}\n")
                    for m in REQUIRED_PASS_MARKERS:
                        if m in l:
                            seen.add(m)
                break

            r, _, _ = select.select([proc.stdout], [], [], 0.5)
            if r:
                line = proc.stdout.readline()
                if not line:
                    break
                sys.stdout.write(f"  [RENODE] {line}")
                sys.stdout.flush()

                for m in REQUIRED_PASS_MARKERS:
                    if m in line:
                        seen.add(m)

                if len(seen) == len(REQUIRED_PASS_MARKERS):
                    passed = True
                    break

        if len(seen) == len(REQUIRED_PASS_MARKERS):
            passed = True
        fail_msg = f"Timed out. Seen {len(seen)}/{len(REQUIRED_PASS_MARKERS)} markers."

    finally:
        print("\n[*] Terminating Renode...")
        try:
            proc.terminate()
            proc.wait(1)
        except:
            proc.kill()

    print("=" * 70)
    print(f"[*] Matched {len(seen)}/{len(REQUIRED_PASS_MARKERS)} milestones")
    for m in REQUIRED_PASS_MARKERS:
        status = "[PASS]" if m in seen else "[MISSING]"
        print(f"   {status} {m}")

    if passed:
        print("[+] INTEGRATION TEST PASSED!")
        print("=" * 70)
        return 0
    else:
        print(f"[-] TEST FAILED: {fail_msg}")
        print("=" * 70)
        return 1

if __name__ == "__main__":
    sys.exit(run_test())
