#!/usr/bin/env python3
#
# The MIT License (MIT)
#
# Copyright (c) 2023 TileDB, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import textwrap

def from_env(key):
  ret = os.getenv(key)
  if not ret:
    print("Missing required environment variable: {}".format(key))
    exit(2)
  return ret

def check_no_file_named(path):
  if os.path.exists(path):
    print("AWS config file '{}' exists. Refusing to overwrite.".format(path))
    exit(2)

def write_config(path):
  check_no_file_named(path)
  endpoint = from_env("R2_S3_ENDPOINT")
  contents = textwrap.dedent("""\
    [default]
    endpoint_url = {}
    region = auto
    output = json
    """)
  with open(path, "w") as handle:
    handle.write(contents.format(endpoint))

def write_creds(path):
  key_id = from_env("R2_ACCESS_KEY_ID")
  key = from_env("R2_SECRET_ACCESS_KEY")
  contents = textwrap.dedent("""\
    [default]
    aws_access_key_id = {}
    aws_secret_access_key = {}
    """)
  with open(path, "w") as handle:
    handle.write(contents.format(key_id, key))

def main():
  home_dir = os.path.expanduser("~")
  aws_dir = os.path.join(home_dir, ".aws")
  if not os.path.exists(aws_dir):
    os.makedirs(aws_dir, exists_ok=True)
  config_file = os.path.join(aws_dir, "config")
  creds_file = os.path.join(aws_dir, "credentials")
  write_config(config_file)
  write_creds(creds_file)

if __name__ == "__main__":
  main()
