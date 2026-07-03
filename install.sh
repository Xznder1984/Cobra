#!/bin/sh
# Cobra Programming Language Installer
# Usage: curl -fsSL https://raw.githubusercontent.com/Xznder1984/Cobra/main/install.sh | sh
set -eu

REPO="Xznder1984/Cobra"
VERSION="${COBRA_VERSION:-latest}"
INSTALL_DIR="${COBRA_INSTALL_DIR:-}"
BANNER_COLOR="\033[38;5;46m"
BOLD="\033[1m"
RESET="\033[0m"
RED="\033[31m"
GREEN="\033[32m"
YELLOW="\033[33m"
BLUE="\033[34m"

# ─── Help / Usage ────────────────────────────────────────────────────────────
usage() {
    cat <<'EOF'
Usage: curl -fsSL https://raw.githubusercontent.com/Xznder1984/Cobra/main/install.sh | sh [flags] [flags]

Flags (passed as environment variables):
  COBRA_VERSION=<version>    Install a specific version (default: latest)
  COBRA_INSTALL_DIR=<path>   Install directory (default: /usr/local/bin)

Alternatively, download the script and run it directly:
  ./install.sh [--help] [--version] [--prefix <path>]
EOF
    exit 0
}

# ─── Flags (when run directly) ──────────────────────────────────────────────
while [ $# -gt 0 ]; do
    case "$1" in
        --help) usage ;;
        --version)
            echo "cobra-installer 1.0.0"
            exit 0
            ;;
        --prefix)
            shift
            INSTALL_DIR="$1"
            ;;
        *) ;;
    esac
    shift
done

# ─── Utilities ───────────────────────────────────────────────────────────────
has_cmd() { command -v "$1" >/dev/null 2>&1; }

cleanup() {
    if [ -n "${TMP_DIR:-}" ] && [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR"
    fi
}
trap cleanup EXIT INT TERM

say() { printf "${BOLD}%s${RESET}\n" "$*"; }
info() { printf "${BLUE}  ->${RESET} %s\n" "$*"; }
ok()   { printf "${GREEN}  [✓]${RESET} %s\n" "$*"; }
warn() { printf "${YELLOW}  [!]${RESET} %s\n" "$*"; }
err()  { printf "${RED}  [✗] %s${RESET}\n" "$*" >&2; }
die()  { err "$1"; exit 1; }

# ─── Banner ──────────────────────────────────────────────────────────────────
banner() {
    cat <<EOF

${BANNER_COLOR}    ╔═══════════════════════════════════════════╗
    ║        Cobra Programming Language        ║
    ║           Fast  |  Safe  |  Modern       ║
    ╚═══════════════════════════════════════════╝${RESET}

EOF
}

# ─── Platform Detection ──────────────────────────────────────────────────────
detect_os() {
    OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
    case "$OS" in
        darwin) OS="darwin" ;;
        linux)  OS="linux" ;;
        *) die "Unsupported operating system: $(uname -s). Only macOS and Linux are supported." ;;
    esac
    info "Detected OS: ${OS}"
}

detect_arch() {
    ARCH="$(uname -m)"
    case "$ARCH" in
        x86_64|amd64) ARCH="x86_64" ;;
        aarch64|arm64) ARCH="arm64" ;;
        *) die "Unsupported architecture: ${ARCH}. Only x86_64 and arm64 are supported." ;;
    esac
    info "Detected architecture: ${ARCH}"
}

# ─── Download Tool ───────────────────────────────────────────────────────────
select_downloader() {
    if has_cmd curl; then
        DOWNLOADER="curl -fsSL"
    elif has_cmd wget; then
        DOWNLOADER="wget -qO-"
    else
        die "Neither curl nor wget found. Install one of them and try again."
    fi
    info "Using downloader: ${DOWNLOADER%% *}"
}

# ─── Existing Installation Check ─────────────────────────────────────────────
check_existing() {
    if has_cmd cobra; then
        CURRENT="$(cobra --version 2>/dev/null || true)"
        if [ -n "$CURRENT" ]; then
            info "Cobra ${CURRENT} is already installed at $(command -v cobra)"
        else
            warn "An existing cobra binary was found but version could not be determined."
        fi
    fi
}

# ─── Version Resolution ──────────────────────────────────────────────────────
resolve_version() {
    if [ "$VERSION" = "latest" ]; then
        info "Fetching latest release version..."
        TAG="$($DOWNLOADER "https://api.github.com/repos/${REPO}/releases/latest" 2>/dev/null \
            | sed -n 's/.*"tag_name": *"\([^"]*\)".*/\1/p' 2>/dev/null || true)"
        if [ -z "$TAG" ]; then
            warn "No GitHub releases found — building from source instead."
            BUILD_FROM_SOURCE=1
            return
        fi
        VERSION="$TAG"
        ok "Latest version: ${VERSION}"
    else
        info "Using specified version: ${VERSION}"
        case "$VERSION" in
            v*) ;;
            *) VERSION="v${VERSION}" ;;
        esac
    fi
}

# ─── Build from Source (fallback when no releases exist) ─────────────────
build_from_source() {
    info "Checking build prerequisites..."
    for cmd in git make; do
        if ! has_cmd "$cmd"; then
            die "Missing prerequisite: ${cmd}. Install it and try again."
        fi
    done
    if ! has_cmd gcc && ! has_cmd clang; then
        die "Missing prerequisite: gcc or clang. Install Xcode Command Line Tools or GCC."
    fi

    TMP_DIR="$(mktemp -d /tmp/cobra-install.XXXXXX)"
    info "Cloning https://github.com/${REPO}.git ..."
    if ! git clone --depth 1 "https://github.com/${REPO}.git" "${TMP_DIR}/Cobra" 2>/dev/null; then
        die "Failed to clone repository."
    fi

    info "Building Cobra from source..."
    (cd "${TMP_DIR}/Cobra" && make build 2>/dev/null) || die "Build failed."

    BINARY="${TMP_DIR}/Cobra/cli/bin/cobra"
    COBRAC="${TMP_DIR}/Cobra/compiler/bin/cobrac"
    RUNTIME="${TMP_DIR}/Cobra/runtime/libcobra_runtime.a"

    for f in "$BINARY" "$COBRAC" "$RUNTIME"; do
        if [ ! -f "$f" ]; then
            die "Required file not found after build: ${f}"
        fi
    done

    chmod +x "$BINARY" "$COBRAC"

    # Install to LIB_DIR, symlink binaries into BIN_DIR
    LIB_DIR="/usr/local/lib/cobra"
    BIN_DIR="${INSTALL_DIR:-/usr/local/bin}"

    mkdir -p "$LIB_DIR" "$BIN_DIR" 2>/dev/null || true

    cp "$BINARY" "$LIB_DIR/cobra"
    cp "$COBRAC" "$LIB_DIR/cobrac"
    cp "$RUNTIME" "$LIB_DIR/libcobra_runtime.a"

    ln -sf "$LIB_DIR/cobra" "$BIN_DIR/cobra" 2>/dev/null || cp "$LIB_DIR/cobra" "$BIN_DIR/cobra"
    ln -sf "$LIB_DIR/cobrac" "$BIN_DIR/cobrac" 2>/dev/null || cp "$LIB_DIR/cobrac" "$BIN_DIR/cobrac"

    # Clean up the clone
    rm -rf "${TMP_DIR}"
    ok "Built from source and installed to ${LIB_DIR}"
}

# ─── Download ────────────────────────────────────────────────────────────────
download_release() {
    TARBALL="cobra-${OS}-${ARCH}.tar.gz"
    URL="https://github.com/${REPO}/releases/download/${VERSION}/${TARBALL}"
    TMP_DIR="$(mktemp -d /tmp/cobra-install.XXXXXX)"

    info "Downloading ${URL} ..."
    if ! $DOWNLOADER "$URL" > "${TMP_DIR}/${TARBALL}" 2>/dev/null; then
        die "Download failed. URL: ${URL}"
    fi
    ok "Downloaded ${TARBALL} (${VERSION})"
}

# ─── Extract ─────────────────────────────────────────────────────────────────
extract_binary() {
    info "Extracting archive..."
    if ! tar -xzf "${TMP_DIR}/${TARBALL}" -C "$TMP_DIR" 2>/dev/null; then
        die "Failed to extract archive. The download may be corrupted."
    fi
    # binary inside could be named "cobra" or be inside a subdirectory
    if [ -f "${TMP_DIR}/cobra" ]; then
        BINARY="${TMP_DIR}/cobra"
    else
        BINARY="$(find "$TMP_DIR" -type f -name 'cobra' 2>/dev/null | head -1)"
        if [ -z "$BINARY" ]; then
            die "cobra binary not found in the archive."
        fi
    fi
    chmod +x "$BINARY"
    ok "Extracted cobra binary"
}

# ─── Install ─────────────────────────────────────────────────────────────────
install_binary() {
    if [ -z "$INSTALL_DIR" ]; then
        TARGET_DIR="/usr/local/bin"
    else
        TARGET_DIR="$INSTALL_DIR"
    fi

    mkdir -p "$TARGET_DIR" 2>/dev/null || true

    if [ -w "$TARGET_DIR" ]; then
        cp "$BINARY" "${TARGET_DIR}/cobra"
    elif has_cmd sudo; then
        info "Writing to ${TARGET_DIR} requires elevated permissions..."
        sudo cp "$BINARY" "${TARGET_DIR}/cobra"
        sudo chmod 755 "${TARGET_DIR}/cobra"
    else
        die "No write permission for ${TARGET_DIR} and sudo is not available."
    fi

    INSTALLED_BIN="${TARGET_DIR}/cobra"
    if [ ! -f "$INSTALLED_BIN" ] || [ ! -x "$INSTALLED_BIN" ]; then
        die "Installation to ${INSTALLED_BIN} failed."
    fi
    ok "Installed cobra to ${INSTALLED_BIN}"
}

# ─── PATH Configuration ──────────────────────────────────────────────────────
shell_config_file() {
    shell="$(basename "${SHELL:-/bin/sh}")"
    case "$shell" in
        zsh) echo "${HOME}/.zshrc" ;;
        bash)
            if [ "$OS" = "darwin" ]; then
                echo "${HOME}/.bash_profile"
            else
                echo "${HOME}/.bashrc"
            fi
            ;;
        fish) echo "${HOME}/.config/fish/config.fish" ;;
        *) echo "" ;;
    esac
}

path_line() {
    case "$(basename "${SHELL:-/bin/sh}")" in
        fish) echo "fish_add_path ${TARGET_DIR}" ;;
        *)    echo "export PATH=\"\${PATH}:${TARGET_DIR}\"" ;;
    esac
}

configure_path() {
    if echo "$PATH" | grep -q "${TARGET_DIR}"; then
        ok "${TARGET_DIR} is already in PATH"
        return
    fi

    CFG="$(shell_config_file)"
    if [ -z "$CFG" ]; then
        warn "Unknown shell (${SHELL}). Add ${TARGET_DIR} to your PATH manually."
        return
    fi

    if [ ! -f "$CFG" ]; then
        # touch the file so we can append to it
        touch "$CFG" 2>/dev/null || true
    fi

    if [ -f "$CFG" ] && [ -w "$CFG" ]; then
        if grep -q "${TARGET_DIR}" "$CFG" 2>/dev/null; then
            ok "${TARGET_DIR} is already configured in ${CFG}"
        else
            line="$(path_line)"
            printf "\n# Added by Cobra installer\n%s\n" "$line" >> "$CFG"
            ok "Added ${TARGET_DIR} to PATH in ${CFG}"
        fi
    else
        warn "Could not write to ${CFG}. Add ${TARGET_DIR} to your PATH manually."
    fi
}

# ─── Verify ──────────────────────────────────────────────────────────────────
verify_installation() {
    if has_cmd cobra; then
        VER="$(cobra --version 2>/dev/null || true)"
        if [ -n "$VER" ]; then
            ok "Cobra ${VER} is installed"
        else
            warn "cobra binary found but version not determined"
        fi
    elif [ -x "$INSTALLED_BIN" ]; then
        VER="$("$INSTALLED_BIN" --version 2>/dev/null || true)"
        if [ -n "$VER" ]; then
            ok "Cobra ${VER} is installed at ${INSTALLED_BIN}"
        else
            ok "Cobra installed at ${INSTALLED_BIN}"
        fi
        warn "Restart your shell or run: export PATH=\"\$PATH:${TARGET_DIR}\""
    else
        die "Verification failed — cobra binary not found."
    fi

    if has_cmd cobra || [ -x "$INSTALLED_BIN" ]; then
        BIN="${INSTALLED_BIN:-$(command -v cobra)}"
        info "Running cobra doctor for a health check..."
        "$BIN" doctor 2>/dev/null && ok "All checks passed!" || warn "cobra doctor reported issues (non-fatal)."
    fi
}

# ─── Welcome ─────────────────────────────────────────────────────────────────
welcome() {
    VER="$(cobra --version 2>/dev/null || "${INSTALLED_BIN}" --version 2>/dev/null || echo "installed")"
    cat <<EOF

${BANNER_COLOR}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}

  ${BOLD}Cobra ${VER} has been installed successfully!${RESET}

  ${BOLD}Get Started${RESET}
    cobra new my-project
    cobra run my-project

  ${BOLD}Documentation${RESET}
    https://cobra-lang.org/docs

  ${BOLD}Community${RESET}
    https://cobra-lang.org/community

${BANNER_COLOR}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}

EOF
}

# ─── Main ────────────────────────────────────────────────────────────────────
main() {
    banner
    check_existing
    detect_os
    detect_arch
    select_downloader
    resolve_version
    if [ "${BUILD_FROM_SOURCE:-0}" = "1" ]; then
        build_from_source
        BINARY="${BIN_DIR}/cobra"
    else
        download_release
        extract_binary
    fi
    install_binary
    configure_path
    verify_installation
    welcome
}

main
