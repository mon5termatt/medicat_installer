#pragma once

namespace medicat {

constexpr int kHelpGateFailureThreshold = 3;

int OperationFailureCount();
void RecordOperationFailure();
void RecordOperationSuccess();
bool NeedsHelpGate();

}  // namespace medicat
