#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

WEB_CLIENT_VERSION="jwc-10.8.9"
WEB_CLIENT_SHA256="4f81cfaf6e8df312aca44a194e23882ebae78f65d98853afa3ec7aceb22d29fa"
WEB_CLIENT_URL="https://github.com/iwalton3/jellyfin-web-jmp/releases/download/${WEB_CLIENT_VERSION}/dist.zip"

function download_compat {
    local output="$1"
    local url="$2"
    local cache_group="$3"
    local download_id

    download_id=$(printf '%s' "$url" | shasum -a 256 | cut -d ' ' -f 1)
    if [[ "${AZ_CACHE:-}" != "" ]]
    then
        if [[ -e "$AZ_CACHE/$cache_group/$download_id" ]]
        then
            echo "Cache hit: $AZ_CACHE/$cache_group/$download_id"
            cp "$AZ_CACHE/$cache_group/$download_id" "$output"
            return
        fi
    fi

    if command -v wget > /dev/null 2>&1
    then
        wget -qO "$output" "$url"
    else
        curl -fL --retry 3 -o "$output" "$url"
    fi

    if [[ "${AZ_CACHE:-}" != "" ]]
    then
        echo "Saving to: $AZ_CACHE/$cache_group/$download_id"
        mkdir -p "$AZ_CACHE/$cache_group/"
        cp "$output" "$AZ_CACHE/$cache_group/$download_id"
    fi
}

function verify_web_client {
    local actual
    actual=$(shasum -a 256 dist.zip | cut -d ' ' -f 1)
    if [[ "$actual" != "$WEB_CLIENT_SHA256" ]]
    then
        echo "Web client checksum mismatch." >&2
        return 1
    fi
}

if [[ "${1:-}" == "--gen-fingerprint" ]]
then
    printf '%s %s\n' "$WEB_CLIENT_VERSION" "$WEB_CLIENT_SHA256" | \
        tee az-cache-fingerprint.list
    exit 0
fi

# Download web client
update_web_client="no"
mkdir -p build
if [[ ! -d "build/dist" ]]
then
    update_web_client="yes"
elif [[ ! -e ".last_wc_version" ]] || \
     [[ "$(cat .last_wc_version)" != "$WEB_CLIENT_VERSION" ]]
then
    update_web_client="yes"
fi

if [[ "$update_web_client" == "yes" ]]
then
    if [[ ! -f dist.zip ]] || ! verify_web_client
    then
        echo "Downloading web client ${WEB_CLIENT_VERSION}..."
        download_compat dist.zip "$WEB_CLIENT_URL" "wc"
    fi
    verify_web_client

    if [[ "${DOWNLOAD_ONLY:-0}" != "1" ]]
    then
        [[ ! -e build/dist ]] || rm -r build/dist
        [[ ! -e dist ]] || rm -r dist
        unzip -q dist.zip
        rm dist.zip
        mv dist build/
    fi
    echo "$WEB_CLIENT_VERSION" > .last_wc_version
fi
