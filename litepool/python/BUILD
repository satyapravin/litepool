# Copyright 2022 Garena Online Private Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@pip_requirements//:requirements.bzl", "requirement")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "protocol",
    srcs = ["protocol.py"],
    deps = [
        requirement("dm-env"),
        requirement("gym"),
        requirement("numpy"),
    ],
)

py_library(
    name = "utils",
    srcs = ["utils.py"],
    deps = [
        requirement("numpy"),
    ],
)

py_library(
    name = "data",
    srcs = ["data.py"],
    deps = [
        requirement("optree"),
        requirement("dm-env"),
        requirement("gym"),
        requirement("gymnasium"),
        requirement("numpy"),
        ":protocol",
    ],
)

py_library(
    name = "env_spec",
    srcs = ["env_spec.py"],
    deps = [
        requirement("dm-env"),
        requirement("gym"),
        requirement("gymnasium"),
        ":data",
        ":protocol",
        ":utils",
    ],
)

py_library(
    name = "litepool",
    srcs = ["litepool.py"],
    deps = [
        requirement("optree"),
        requirement("dm-env"),
        requirement("numpy"),
        requirement("packaging"),
        ":protocol",
    ],
)

py_library(
    name = "dm_litepool",
    srcs = ["dm_litepool.py"],
    deps = [
        requirement("optree"),
        requirement("dm-env"),
        requirement("numpy"),
        ":data",
        ":litepool",
        ":utils",
    ],
)

py_library(
    name = "gym_litepool",
    srcs = ["gym_litepool.py"],
    deps = [
        requirement("optree"),
        requirement("dm-env"),
        requirement("gym"),
        requirement("numpy"),
        ":data",
        ":litepool",
        ":utils",
    ],
)

py_library(
    name = "gymnasium_litepool",
    srcs = ["gymnasium_litepool.py"],
    deps = [
        requirement("optree"),
        requirement("dm-env"),
        requirement("gymnasium"),
        requirement("numpy"),
        ":data",
        ":litepool",
        ":utils",
    ],
)

py_library(
    name = "api",
    srcs = ["api.py"],
    deps = [
        ":dm_litepool",
        ":env_spec",
        ":gym_litepool",
        ":gymnasium_litepool",
        ":protocol",
    ],
)

py_library(
    name = "python",
    srcs = ["__init__.py"],
    deps = [
        ":api",
    ],
)
