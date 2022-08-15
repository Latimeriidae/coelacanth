#!/usr/bin/env python3

import argparse
import os
import re
import sys


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Find appropriate conan profile matching query.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("query", type=str, help="regex query for desired profile")
    args = parser.parse_args(sys.argv[1:])
    query = args.query
    query_compiled = re.compile(query)
    default_profile = "default"

    profiles = os.popen("conan profile list").readlines()
    profiles = [profile.strip() for profile in profiles]
    filtered_profiles = [p for p in profiles if query_compiled.search(p)]
    if len(filtered_profiles) == 0:
        eprint("WARNING: No matching conan profiles found for query '{}'.".format(query))
        eprint("Will try to use default profile.")
        if default_profile not in profiles:
            eprint("ERROR: Default profile '{}' not found.".format(default_profile))
            eprint("Did you forget to run 'conan config init'?")
            sys.exit(1)
        print(default_profile)
        sys.exit(0)
    if len(filtered_profiles) > 1:
        eprint("WARNING: Found more than one matching profile for query '{}':".format(query))
        eprint("\n".join(filtered_profiles))
        eprint("Will be picked the first one.")
    print(filtered_profiles[0])
