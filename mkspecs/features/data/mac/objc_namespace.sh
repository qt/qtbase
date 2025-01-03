#!/bin/bash
# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

script_argument_prefix="-Wobjc_namespace,--"

required_arguments="target suffix original_ld"
optional_arguments="exclude_list exclude_regex silent"

for argument in $required_arguments $optional_arguments; do
    declare "$argument="
done

declare -i silent=0
declare -a linker_arguments

for i in "$@"; do
    case $1 in
        $script_argument_prefix*)
            declare "${1#$script_argument_prefix}"
            ;;
        -o)
            if [ -n "$2" ]; then
                target="$2"
            fi
            linker_arguments+=("$1")
            ;;
        *)
            linker_arguments+=("$1")
            ;;
    esac
    shift
done

get_entry() {
    local map=$1 key=$2
    local i="${map}_map_${2}"
    printf '%s' "${!i}"
}

error() {
    echo "$0: error: $*" >&2
    exit 1
}

for argument in $required_arguments; do
    if [ -z "${!argument}" ]; then
        error "missing argument --${argument}"
    fi
done

# Normalize suffix so we can use it as a bash variable
suffix=${suffix//[-. ]/_}

link_binary() {
    (PS4=; test $silent -ne 1 && set -x; $original_ld "${linker_arguments[@]}" "$@") 2>&1 || exit 1
}

sanitize_address() {
    local address="$1"
    address=${address#0x} # Remove hex prefix
    address=${address: ${#address} < 8 ? 0 : -8} # Limit to 32-bit
    echo "0x$address"
}

arch_offset=0
read_binary() {
    local address=$1
    local length=$2

    seek=$(($address + $arch_offset))
    dd if="$target" bs=1 iseek=$seek count=$length 2>|/dev/null
}

read_32bit_value() {
    local address=$1
    read_binary $address 4 | xxd -p | dd conv=swab 2>/dev/null | rev
}

otool_args=
otool() {
    command otool $otool_args "$@"
}

declare -a extra_classnames_files

inspect_binary() {
    inspect_mode="$1"

    classnames_section="__objc_classname"
    classnames=$(otool -v -s __TEXT $classnames_section "$target" | tail -n +3)
    if [ -z "$classnames" ]; then
        echo " ℹ️  No Objective-C classes found in binary"
        return 1
    fi

    while read -a classname; do
        address=$(sanitize_address ${classname[0]})
        name=${classname[1]}

        declare "address_to_classname_map_$address=$name"
        declare "classname_to_address_map_$name=$address"
    done <<< "$classnames"

    extra_classnames_file="$(mktemp -t ${classnames_section}_additions).S"
    extra_classnames_files+=("$extra_classnames_file")

    if [ "$inspect_mode" == "inject_classnames" ]; then
        echo " ℹ️  Class names have not been namespaced, adding suffix '$suffix'..."
        printf ".section __TEXT,$classnames_section,cstring_literals,no_dead_strip\n" > $extra_classnames_file
    elif [ "$inspect_mode" == "patch_classes" ]; then
        echo " ℹ️  Found namespaced class names, updating class entries..."
    fi

    classes=$(otool -o -v "$target" | grep "OBJC_CLASS_RO\|OBJC_METACLASS_RO")
    if [ -z "$classes" ]; then
        echo " 💥  Failed to read class entries from binary"
        exit 1
    fi

    while read -a class; do
        address="$(sanitize_address ${class[1]})"
        class_flags="0x$(read_32bit_value $address)"
        if [ -z "$class_flags" ]; then
            echo " 💥  Failed to read class flags for class at $address"
            continue
        fi

        is_metaclass=$(($class_flags & 0x1))

        name_offset=$(($address + 24))
        classname_address="0x$(read_32bit_value $name_offset)"
        if [ -z "$classname_address" ]; then
            echo " 💥  Failed to read class name address for class at $address"
            continue
        fi

        classname=$(get_entry address_to_classname $classname_address)
        if [ -z "$classname" ]; then
            echo " 💥  Failed to resolve class name for address '$classname_address'"
            continue
        fi

        if [[ $exclude_list =~ $classname || $classname =~ $exclude_regex ]]; then
            if [ $is_metaclass -eq 1 ]; then
                class_type="meta class"
            else
                class_type="class"
            fi
            echo " 🚽  Skipping excluded $class_type '$classname'"
            continue
        fi

        newclassname="${classname}_${suffix}"

        if [ "$inspect_mode" == "inject_classnames" ]; then
            if [ $is_metaclass -eq 1 ]; then
                continue
            fi

            echo " 💉  Injecting $classnames_section entry '$newclassname' for '$classname'"
            printf ".asciz \"$newclassname\"\n" >> $extra_classnames_file

        elif [ "$inspect_mode" == "patch_classes" ]; then
            newclassname_address=$(get_entry classname_to_address ${newclassname})
            if [ -z "$newclassname_address" ]; then
                echo " 💥  Failed to resolve class name address for class '$newclassname'"
                continue
            fi

            if [ $is_metaclass -eq 1 ]; then
                class_type="meta"
            else
                class_type="class"
            fi

            name_offset=$(($name_offset + $arch_offset))

            echo " 🔨  Patching class_ro_t at $address ($class_type) from $classname_address ($classname) to $newclassname_address ($newclassname)"
            echo ${newclassname_address: -8} | rev | dd conv=swab 2>/dev/null | xxd -p -r -seek $name_offset -l 4 - "$target"
        fi
    done <<< "$classes"
}

echo "🔩  Linking binary using '$original_ld'..."
link_binary

echo "🔎  Inspecting binary '$target'..."
if [ ! -f "$target" ]; then
    echo " 💥  Target does not exist!"
    exit 1
fi

read -a mach_header <<< "$(otool -h "$target" -v | tail -n 1)"
if [ "${mach_header[0]}" != "MH_MAGIC_64" ]; then
    echo " 💥  Binary is not 64-bit, only 64-bit binaries are supported!"
    exit 1
fi

architectures=$(otool -f -v "$target" | grep architecture)

setup_arch() {
    arch="$1"
    if [ ! -z "$arch" ]; then
        otool_args="-arch $arch"
        offset=$(otool -f -v "$target" | grep -A 6 "architecture $arch" | grep offset)
        offset="${offset##*( )}"
        arch_offset="$(echo $offset | cut -d ' ' -f 2)"
        echo "🤖 Processing architecture '$arch' at offset $arch_offset..."
    fi
}

while read -a arch; do
    setup_arch "${arch[1]}"
    inspect_binary inject_classnames
    if [ $? -ne 0 ]; then
        exit
    fi
done <<< "$architectures"

echo "🔩  Re-linking binary with extra __objc_classname section(s)..."
link_binary "${extra_classnames_files[@]}"

while read -a arch; do
    setup_arch "${arch[1]}"
    inspect_binary patch_classes
done <<< "$architectures"
