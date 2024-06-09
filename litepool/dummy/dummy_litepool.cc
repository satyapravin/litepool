/*
 * Copyright 2021 Garena Online Private Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "litepool/dummy/dummy_litepool.h"

#include "litepool/core/py_litepool.h"

/**
 * Wrap the `DummyEnvSpec` and `DummyLitePool` with the corresponding `PyEnvSpec`
 * and `PyLitePool` template.
 */
using DummyEnvSpec = PyEnvSpec<dummy::DummyEnvSpec>;
using DummyLitePool = PyLitePool<dummy::DummyLitePool>;

/**
 * Finally, call the REGISTER macro to expose them to python
 */
PYBIND11_MODULE(dummy_litepool, m) { REGISTER(m, DummyEnvSpec, DummyLitePool) }
