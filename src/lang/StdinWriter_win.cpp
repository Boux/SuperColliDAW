#include "StdinWriter.h"

namespace supercollidaw {

// Windows raises no signal on a broken pipe; the write just fails.
void StdinWriter::blockBrokenPipeSignal() {}

}
