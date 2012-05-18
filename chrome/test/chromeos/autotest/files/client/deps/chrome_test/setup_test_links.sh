#!/bin/bash
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# A script to setup symbolic links needed for Chrome's automated UI tests.
# Should be run on test image after installing the 'chrome_test' and
# 'pyauto_dep' dependencies.

# TODO(rkc): Figure out why does the sym linking break tests, eventually
this_dir="$(realpath $(dirname $0))"
[ -f "$this_dir/chrome" ] || cp "/opt/google/chrome/chrome" "$this_dir/chrome"
ln -f -s "/opt/google/chrome/chrome.pak" "$this_dir/chrome.pak"
[ -L "$this_dir/locales" ] || ln -f -s "/opt/google/chrome/locales" \
    "$this_dir/locales"
[ -L "$this_dir/resources" ] || ln -f -s "/opt/google/chrome/resources" \
    "$this_dir/resources"
ln -f -s "/opt/google/chrome/resources.pak" "$this_dir/resources.pak"
ln -f -s "/opt/google/chrome/ui_resources_standard.pak" \
    "$this_dir/ui_resources_standard.pak"
ln -f -s "/opt/google/chrome/ui_resources_touch.pak" \
    "$this_dir/ui_resources_touch.pak"
ln -f -s "/opt/google/chrome/theme_resources_standard.pak" \
    "$this_dir/theme_resources_standard.pak"
ln -f -s "/opt/google/chrome/theme_resources_touch_1x.pak" \
    "$this_dir/theme_resources_touch_1x.pak"

mkdir -p "$this_dir/chromeos"
ln -f -s "/opt/google/chrome/chromeos/libcros.so" \
    "$this_dir/chromeos/libcros.so"


# Create links to resources from pyauto_dep.

chrome_test_dep_dir=$(realpath "$this_dir/../../..")
pyauto_dep_dir=$(realpath "$chrome_test_dep_dir/../pyauto_dep")
if [ ! -d "$pyauto_dep_dir" ]; then
    echo "'pyauto_dep' dependency is missing. Install it first." >&2
    exit 1
fi

# Create symlink(s) to the given abs path(s) in 'pyauto_dep' dependency
# pointing from the corresponding item in 'chrome_test' dependency
# (if the destination doesn't already exist). Intermediate dirs are created
# if necessary.
function link_from_pyauto_dep() {
    for src_path in $@; do
        [[ "$src_path" == "/usr/local/autotest/deps/pyauto_dep"/* ]] || return
        dest_path="${src_path/pyauto_dep/chrome_test}"
        mkdir -p "$(dirname "$dest_path")"
        [ -e "$dest_path" ] || ln -f -s $src_path $dest_path
    done
}

link_from_pyauto_dep \
    "$pyauto_dep_dir/test_src/chrome/test/pyautolib" \
    "$pyauto_dep_dir/test_src/net/tools/testserver" \
    "$pyauto_dep_dir/test_src/out/Release/chromedriver" \
    "$pyauto_dep_dir/test_src/out/Release/pyautolib.py" \
    "$pyauto_dep_dir/test_src/out/Release/pyproto" \
    "$pyauto_dep_dir/test_src/out/Release/suid-python" \
    "$pyauto_dep_dir/test_src/out/Release/_pyautolib.so" \
    "$pyauto_dep_dir/test_src/third_party"/*
