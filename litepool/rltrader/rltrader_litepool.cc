
#include "litepool/rltrader/rltrader_litepool.h"

#include "litepool/core/py_litepool.h"

/**
 * Wrap the `RlTraderEnvSpec` and `RlTraderLitePool` with the corresponding `PyEnvSpec`
 * and `PyLitePool` template.
 */
using RlTraderEnvSpec = PyEnvSpec<rltrader::RlTraderEnvSpec>;
using RlTraderLitePool = PyLitePool<rltrader::RlTraderLitePool>;

/**
 * Finally, call the REGISTER macro to expose them to python
 */
PYBIND11_MODULE(rltrader_litepool, m) { REGISTER(m, RlTraderEnvSpec, RlTraderLitePool) }
