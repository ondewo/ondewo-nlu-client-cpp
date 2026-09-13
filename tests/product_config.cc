#include "product_config.h"

namespace ondewo_client_test {

// Mirrors the #include list of the generated public-api.h, one .proto per pair of headers:
//   sed -n 's|^#include "\(.*\)\.pb\.h"$|\1|p' public-api.h | sed 's|\.grpc$||' | sort -u
const std::vector<std::string> kProtoFileNames = {
    "google/api/annotations.proto",
    "google/api/http.proto",
    "google/rpc/status.proto",
    "google/type/latlng.proto",
    "ondewo/nlu/agent.proto",
    "ondewo/nlu/aiservices.proto",
    "ondewo/nlu/ccai_project.proto",
    "ondewo/nlu/common.proto",
    "ondewo/nlu/context.proto",
    "ondewo/nlu/entity_type.proto",
    "ondewo/nlu/intent.proto",
    "ondewo/nlu/llm_evaluation.proto",
    "ondewo/nlu/operation_metadata.proto",
    "ondewo/nlu/operations.proto",
    "ondewo/nlu/project_role.proto",
    "ondewo/nlu/project_statistics.proto",
    "ondewo/nlu/rag.proto",
    "ondewo/nlu/server_statistics.proto",
    "ondewo/nlu/session.proto",
    "ondewo/nlu/user.proto",
    "ondewo/nlu/utility.proto",
    "ondewo/nlu/webhook.proto",
    "ondewo/qa/qa.proto",
};

const std::vector<std::string> kServiceFullNames = {
    "ondewo.nlu.Agents",
    "ondewo.nlu.AiServices",
    "ondewo.nlu.CcaiProjects",
    "ondewo.nlu.Contexts",
    "ondewo.nlu.EntityTypes",
    "ondewo.nlu.Intents",
    "ondewo.nlu.LlmEvaluations",
    "ondewo.nlu.Operations",
    "ondewo.nlu.ProjectRoles",
    "ondewo.nlu.ProjectStatistics",
    "ondewo.nlu.Rags",
    "ondewo.nlu.ServerStatistics",
    "ondewo.nlu.Sessions",
    "ondewo.nlu.Users",
    "ondewo.nlu.Utilities",
    "ondewo.nlu.Webhook",
    "ondewo.qa.QA",
};

const std::vector<ExpectedMethod> kExpectedMethods = {
    // The complete CRUD surface of one small service ...
    {"ondewo.nlu.Contexts", "ListContexts"},
    {"ondewo.nlu.Contexts", "GetContext"},
    {"ondewo.nlu.Contexts", "CreateContext"},
    {"ondewo.nlu.Contexts", "UpdateContext"},
    {"ondewo.nlu.Contexts", "DeleteContext"},
    {"ondewo.nlu.Contexts", "DeleteAllContexts"},
    // ... the two RPCs the product exists for, one of them server/bidi-streaming ...
    {"ondewo.nlu.Sessions", "DetectIntent"},
    {"ondewo.nlu.Sessions", "StreamingDetectIntent"},
    // ... a long-running operation returning google.longrunning.Operation ...
    {"ondewo.nlu.Agents", "TrainAgent"},
    {"ondewo.nlu.Agents", "ExportAgent"},
    // ... and one RPC from the second proto package in the same library.
    {"ondewo.qa.QA", "GetAnswer"},
};

const std::string kScalarMessageFullName = "ondewo.nlu.Context";

const std::string kEnumFullName = "ondewo.nlu.IntentView";

// ONDEWO NLU API 7.1.0 generates 715 messages (map entries excluded) and 83 enums across
// the files listed above. The floors sit just below that.
const int kMinimumMessageCount = 700;
const int kMinimumEnumCount = 80;

}  // namespace ondewo_client_test
