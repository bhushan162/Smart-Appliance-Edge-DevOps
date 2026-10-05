# Concept: Docker Best Practices for Embedded Toolchains

**Date:** 2026-10-05
**Topic:** Hardening a Zephyr/CMake Docker Build Environment

During the creation of the Stage 1 Zephyr build container, I encountered several classic Docker and Linux architectural pitfalls. This document serves as a checklist of what *not* to do when writing production Dockerfiles.

### 1. The `sudo` Anti-Pattern
* **The Mistake:** Writing `RUN sudo apt install...`
* **The Reality:** By default, Docker containers execute all commands as the `root` user. The `sudo` package is usually not even installed on minimal base images (like Ubuntu). 
* **The Fix:** Omit `sudo` entirely. Just use `RUN apt-get install...`.

### 2. Bash Chaining vs. Line Breaks (`\` vs `&& \`)
* **The Mistake:** Using only a backslash `\` to string commands together.
    ```dockerfile
    RUN west init /opt/zephyrproject \
        cd /opt/zephyrproject \
        west update
    ```
* **The Reality:** In Bash, a trailing `\` just means "ignore the enter key and continue this command." The shell interprets the above as one broken command: `west init /opt/zephyrproject cd /opt/zephyrproject west update`.
* **The Fix:** Use `&& \`. The `&&` tells Bash "run the next command ONLY if this one succeeds," and the `\` formats it cleanly on a new line. 

### 3. Sourcing Virtual Environments in Docker
* **The Mistake:** Trying to activate a Python venv using `RUN source /opt/venv/bin/activate` (or `RUN PATH/activate`).
* **The Reality:** Every `RUN` command in a Dockerfile spawns a brand new, temporary shell. If you activate a venv in one `RUN` step, it is immediately forgotten in the next.
* **The Fix:** Modify the global environment variables using `ENV`. By prepending the venv's bin folder to the path (`ENV PATH="/opt/venv/bin:$PATH"`), the virtual environment becomes permanently active for all future Docker layers.

### 4. Breaking Separation of Concerns (The `COPY` Trap)
* **The Mistake:** Baking the firmware source code directly into the build environment image using `COPY . ../firmware/.`.
* **The Reality:** A build environment image (which contains compilers and SDKs) should be static and reusable. If you `COPY` your code into the Dockerfile, you have to rebuild a 5GB Docker image every time you change a single line of C code. 
* **The Fix:** Keep the Docker image generic. Store the code locally, and use a CI orchestrator (like Jenkins) or a volume mount (`docker run -v`) to inject the code into the container at runtime.

### 5. Interactive Prompts Hanging the Build
* **The Mistake:** Running `apt-get install` without disabling interactive prompts.
* **The Reality:** Certain Linux packages (like `tzdata`) will pause the installation to ask the user to select their geographic timezone. Because a Docker build is "headless" (no keyboard attached), the build will freeze indefinitely waiting for input.
* **The Fix:** Always declare `ARG DEBIAN_FRONTEND=noninteractive` before running `apt-get install`.

### 6. SDK Image Bloat
* **The Mistake:** Running a blanket `west sdk install`.
* **The Reality:** The Zephyr SDK contains compilers for every architecture it supports (x86, RISC-V, ARC, Xtensa, ARM). Installing all of them results in a massive 15GB+ image.
* **The Fix:** Scope the installation to the specific hardware target using `--toolchains arm-zephyr-eabi`.