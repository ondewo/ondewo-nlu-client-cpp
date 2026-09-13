// Assertions against the concrete C++ types the NLU stubs generate.
//
// This is the per-product half of the suite: it names ondewo::nlu types, so replicating
// the suite to another ONDEWO client means rewriting this file against that product's
// messages and services. Everything generic lives in test_generated_stubs.cc.

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "ondewo/nlu/agent.pb.h"
#include "ondewo/nlu/context.grpc.pb.h"
#include "ondewo/nlu/context.pb.h"
#include "ondewo/nlu/intent.pb.h"
#include "ondewo/nlu/session.grpc.pb.h"
#include "ondewo/qa/qa.grpc.pb.h"

namespace ondewo_client_test {
namespace {

// A channel to a port nothing listens on. gRPC connects lazily, so constructing stubs
// against it touches no network at all; the one test that does issue an RPC gives it a
// short deadline and asserts only that the call comes back as a failure.
std::shared_ptr<grpc::Channel> DeadChannel() {
  return grpc::CreateChannel("127.0.0.1:1", grpc::InsecureChannelCredentials());
}

TEST(TypedApi, MessageSurvivesSerializeAndParse) {
  ondewo::nlu::Context original;
  original.set_name("i-am-a-context");
  original.set_lifespan_count(7);

  ondewo::nlu::Context::Parameter parameter;
  parameter.set_name("city");
  parameter.set_display_name("City");
  parameter.set_value("Vienna");
  (*original.mutable_parameters())["city"] = parameter;

  std::string bytes;
  ASSERT_TRUE(original.SerializeToString(&bytes));
  EXPECT_FALSE(bytes.empty());

  ondewo::nlu::Context parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));

  EXPECT_EQ(parsed.name(), "i-am-a-context");
  EXPECT_EQ(parsed.lifespan_count(), 7);
  ASSERT_EQ(parsed.parameters().size(), 1u);
  ASSERT_EQ(parsed.parameters().count("city"), 1u);
  EXPECT_EQ(parsed.parameters().at("city").value(), "Vienna");
  EXPECT_EQ(parsed.SerializeAsString(), bytes);
}

// `optional float lifespan_time = 4` has proto3 explicit presence. Set to 0 - the type's
// default - it must still reach the wire and still read back as *present*; a generator
// that drops the presence bit makes 0 unsendable, which is exactly the class of bug that
// hit the Angular client.
TEST(TypedApi, ExplicitPresenceFieldSurvivesItsZeroValue) {
  ondewo::nlu::Context original;
  EXPECT_FALSE(original.has_lifespan_time());

  original.set_lifespan_time(0.0F);
  ASSERT_TRUE(original.has_lifespan_time());

  const std::string bytes = original.SerializeAsString();
  EXPECT_FALSE(bytes.empty()) << "an explicitly present 0.0 was not written to the wire";

  ondewo::nlu::Context parsed;
  ASSERT_TRUE(parsed.ParseFromString(bytes));
  EXPECT_TRUE(parsed.has_lifespan_time()) << "presence of a 0.0 value was lost on the wire";
  EXPECT_FLOAT_EQ(parsed.lifespan_time(), 0.0F);

  original.clear_lifespan_time();
  EXPECT_FALSE(original.has_lifespan_time());
  EXPECT_TRUE(original.SerializeAsString().empty());
}

// A plain (non-optional) proto3 scalar has the opposite contract: its zero value is the
// default and must NOT be written. Asserting both directions is what proves the two field
// kinds really are generated differently.
TEST(TypedApi, PlainScalarZeroValueStaysOffTheWire) {
  ondewo::nlu::Context context;
  context.set_lifespan_count(0);
  EXPECT_TRUE(context.SerializeAsString().empty());

  context.set_lifespan_count(1);
  EXPECT_FALSE(context.SerializeAsString().empty());
}

TEST(TypedApi, EnumZeroValueIsTheUnspecifiedOne) {
  EXPECT_EQ(static_cast<int>(ondewo::nlu::IntentView::INTENT_VIEW_UNSPECIFIED), 0);
  EXPECT_EQ(ondewo::nlu::IntentView_Name(ondewo::nlu::IntentView::INTENT_VIEW_UNSPECIFIED),
            "INTENT_VIEW_UNSPECIFIED");
  EXPECT_EQ(static_cast<int>(ondewo::nlu::AgentView::AGENT_VIEW_UNSPECIFIED), 0);

  ondewo::nlu::IntentView parsed = ondewo::nlu::IntentView::INTENT_VIEW_FULL;
  ASSERT_TRUE(ondewo::nlu::IntentView_Parse("INTENT_VIEW_UNSPECIFIED", &parsed));
  EXPECT_EQ(parsed, ondewo::nlu::IntentView::INTENT_VIEW_UNSPECIFIED);

  // A request defaults to the zero view, so the zero value has to be requestable.
  ondewo::nlu::ListIntentsRequest request;
  EXPECT_EQ(request.intent_view(), ondewo::nlu::IntentView::INTENT_VIEW_UNSPECIFIED);
}

TEST(TypedApi, ServiceStubsAreConstructibleAgainstAChannel) {
  const std::shared_ptr<grpc::Channel> channel = DeadChannel();
  ASSERT_NE(channel, nullptr);

  std::unique_ptr<ondewo::nlu::Contexts::Stub> contexts =
      ondewo::nlu::Contexts::NewStub(channel);
  std::unique_ptr<ondewo::nlu::Sessions::Stub> sessions =
      ondewo::nlu::Sessions::NewStub(channel);
  std::unique_ptr<ondewo::qa::QA::Stub> qa = ondewo::qa::QA::NewStub(channel);

  EXPECT_NE(contexts, nullptr);
  EXPECT_NE(sessions, nullptr);
  EXPECT_NE(qa, nullptr);
}

TEST(TypedApi, ServicesKeepTheirFullyQualifiedNames) {
  EXPECT_STREQ(ondewo::nlu::Contexts::service_full_name(), "ondewo.nlu.Contexts");
  EXPECT_STREQ(ondewo::nlu::Sessions::service_full_name(), "ondewo.nlu.Sessions");
  EXPECT_STREQ(ondewo::qa::QA::service_full_name(), "ondewo.qa.QA");
}

// Actually issue an RPC. Nothing is listening, so the only correct outcome is a failure -
// but reaching a transport-level failure means the stub, the request/response types and
// the generated method descriptor all linked and dispatched. A crash or an OK here would
// mean the generated client is broken.
TEST(TypedApi, UnaryRpcAgainstADeadEndpointFailsCleanly) {
  std::unique_ptr<ondewo::nlu::Contexts::Stub> contexts =
      ondewo::nlu::Contexts::NewStub(DeadChannel());

  grpc::ClientContext client_context;
  client_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

  ondewo::nlu::GetContextRequest request;
  request.set_name("projects/p/agent/sessions/s/contexts/c");
  ondewo::nlu::Context response;

  const grpc::Status status = contexts->GetContext(&client_context, request, &response);

  EXPECT_FALSE(status.ok()) << "an RPC to a dead endpoint reported success";
  EXPECT_TRUE(status.error_code() == grpc::StatusCode::UNAVAILABLE ||
              status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
      << "unexpected status " << status.error_code() << ": " << status.error_message();
}

// StreamingDetectIntent is bidirectional, so it gets its own generated ClientReaderWriter
// type. Driving one proves that half of the generated service compiled and dispatches too.
TEST(TypedApi, BidiStreamingRpcStubIsUsable) {
  std::unique_ptr<ondewo::nlu::Sessions::Stub> sessions =
      ondewo::nlu::Sessions::NewStub(DeadChannel());

  grpc::ClientContext client_context;
  client_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

  std::unique_ptr<grpc::ClientReaderWriter<ondewo::nlu::StreamingDetectIntentRequest,
                                           ondewo::nlu::StreamingDetectIntentResponse>>
      stream(sessions->StreamingDetectIntent(&client_context));
  ASSERT_NE(stream, nullptr);

  ondewo::nlu::StreamingDetectIntentRequest request;
  request.set_session("a-session");
  request.set_single_utterance(true);
  stream->Write(request);
  stream->WritesDone();

  ondewo::nlu::StreamingDetectIntentResponse response;
  EXPECT_FALSE(stream->Read(&response)) << "a dead endpoint returned a streamed response";

  const grpc::Status status = stream->Finish();
  EXPECT_FALSE(status.ok()) << "a stream to a dead endpoint reported success";
}

}  // namespace
}  // namespace ondewo_client_test
