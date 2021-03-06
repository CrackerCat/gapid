/*
 * Copyright (C) 2018 Google Inc.
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

#ifndef GAPIR_REPLAY_CONNECTION_H
#define GAPIR_REPLAY_CONNECTION_H

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace grpc {
template <typename RES, typename REQ>
class ServerReaderWriter;
}

namespace replay_service {
class Payload;
class Resources;
class ReplayRequest;
class PayloadRequest;
class ResourceRequest;
class PostData;
class ReplayResponse;
class Notification;
}  // namespace replay_service

namespace gapir {

using ReplayGrpcStream =
    grpc::ServerReaderWriter<replay_service::ReplayResponse,
                             replay_service::ReplayRequest>;
using PayloadHandler = std::function<bool(const replay_service::Payload&)>;
using ResourcesHandler = std::function<bool(const replay_service::Resources&)>;

// ReplayConnection wraps the replay stream connection and provides an interface
// to ease receiving and sending of replay data, hides the protobuf and grpc
// detailed code.
class ReplayConnection {
 public:
  // ResourceRequest is a wraper class of replay_service::ResourceRequest, it
  // hides the new/delete operations of the proto object from the outer code.
  class ResourceRequest {
   public:
    // Returns a new created empty ResourceRequest.
    static std::unique_ptr<ResourceRequest> create() {
      return std::unique_ptr<ResourceRequest>(new ResourceRequest());
    }

    ~ResourceRequest();

    ResourceRequest(const ResourceRequest&) = delete;
    ResourceRequest(ResourceRequest&&) = delete;
    ResourceRequest& operator=(const ResourceRequest&) = delete;
    ResourceRequest& operator=(ResourceRequest&&) = delete;

    // Adds a resource, with its ID and expected size, to the request list.
    bool append(const std::string& id, size_t size);
    // Get the internal proto object raw pointer, and gives away the ownership
    // of the proto object.
    replay_service::ResourceRequest* release_to_proto();

   private:
    ResourceRequest();

    // The internal wrapped proto object.
    std::unique_ptr<replay_service::ResourceRequest> mProtoResourceRequest;
  };

  // Posts is a wraper class of replay_service::PostData, it hides the
  // new/delete operations of the proto object from the outer code.
  class Posts {
   public:
    // Returns a new created empty Posts
    static std::unique_ptr<Posts> create() {
      return std::unique_ptr<Posts>(new Posts());
    }

    ~Posts();

    Posts(const Posts&) = delete;
    Posts(Posts&&) = delete;
    Posts& operator=(const Posts&) = delete;
    Posts& operator=(Posts&&) = delete;

    // Appends a new piece of post data to the posts.
    bool append(uint64_t id, const void* data, size_t size);
    // Gets the raw pointer of the internal proto object, and gives away the
    // ownership of the proto object.
    replay_service::PostData* release_to_proto();
    // Returns the number of pieces of post data.
    size_t piece_count() const;
    // Returns size in bytes of the 'index'th (starts from 0) piece of post
    // data.
    size_t piece_size(int index) const;
    // Returns a pointer to the data of the 'index'th (starts from 0) piece of
    // post data.
    const void* piece_data(int index) const;
    // Returns the ID of the 'index'th (starts from 0) piece of post data.
    uint64_t piece_id(int index) const;

   private:
    Posts();

    // The internal proto object.
    std::unique_ptr<replay_service::PostData> mProtoPostData;
  };

  // Payload is a wraper class of replay_service::Payload, it hides the
  // new/delete operations of the proto object from outer code.
  class Payload {
   public:
    // Gets a Payload from replay connection stream. Takes the ownership of
    // the proto object in the returned Payload. Returns nullptr in case of
    // error.
    static std::unique_ptr<Payload> get(ReplayGrpcStream* stream);

    // Creates a new Payload from a protobuf payload object.
    Payload(std::unique_ptr<replay_service::Payload> protoPayload);

    ~Payload();
    Payload(const Payload&) = delete;
    Payload(Payload&&) = delete;
    Payload& operator=(const Payload&) = delete;
    Payload& operator=(Payload&&) = delete;

    // Returns the stack size in bytes specified by this replay payload.
    uint32_t stack_size() const;
    // Returns the volatile memory size in bytes specified by this replay
    // payload.
    uint32_t volatile_memory_size() const;
    // Returns the constant memory size in bytes specified by this replay
    // payload.
    size_t constants_size() const;
    // Gets a pointer to the payload constant data.
    const void* constants_data() const;
    // Returns the count of resource info.
    size_t resource_info_count() const;
    // Returns the ID of the 'index'th (starts from 0) resource info.
    const std::string resource_id(int index) const;
    // Returns the expected size of the 'index'th (starts from 0) resource info.
    uint32_t resource_size(int index) const;
    // Returns the size in bytes of the opcodes in this replay payload.
    size_t opcodes_size() const;
    // Gets a pointer to the opcodes in this replay payload.
    const void* opcodes_data() const;

   private:
    Payload(std::unique_ptr<replay_service::ReplayRequest> req);

    // The internal proto object.
    std::unique_ptr<replay_service::ReplayRequest> mProtoReplayRequest;
  };

  // Resources is a wraper class of replay_service::Resources, it hides the
  // new/delete operations of the proto object from outer code.
  class Resources {
   public:
    // Gets a Resources from the replay connection stream, takes the ownership
    // of the proto object received. Returns nullptr in case of error.
    static std::unique_ptr<Resources> get(ReplayGrpcStream* stream);

    // Creates a new Resources from a protobuf resources object.
    Resources(std::unique_ptr<replay_service::Resources> protoResources);

    ~Resources();
    Resources(const Resources&) = delete;
    Resources(Resources&&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources& operator=(Resources&&) = delete;

    // Returns the size in bytes of the data contained by this Resources.
    size_t size() const;
    // Gets a pointer to the data contained by this Resources.
    const void* data() const;

   private:
    Resources(std::unique_ptr<replay_service::ReplayRequest> req);

    // The internal proto object.
    std::unique_ptr<replay_service::ReplayRequest> mProtoReplayRequest;
  };

  // Creates a ReplayConnection from the gRPC stream. If the gRPC stream is
  // nullptr, returns nullptr
  static std::unique_ptr<ReplayConnection> create(ReplayGrpcStream* stream) {
    if (stream == nullptr) {
      return nullptr;
    }
    return std::unique_ptr<ReplayConnection>(new ReplayConnection(stream));
  }

  virtual ~ReplayConnection();

  ReplayConnection(const ReplayConnection&) = delete;
  ReplayConnection(ReplayConnection&&) = delete;
  ReplayConnection& operator=(const ReplayConnection&) = delete;
  ReplayConnection& operator=(ReplayConnection&&) = delete;

  // Sends PayloadRequest and returns the received Payload. Returns nullptr in
  // case of error.
  virtual std::unique_ptr<Payload> getPayload();
  // Sends ResourceRequest and returns the received Resources. Returns nullptr
  // in case of error.
  virtual std::unique_ptr<Resources> getResources(
      std::unique_ptr<ResourceRequest> req);

  // Sends ReplayFinished signal. Returns true if succeeded, otherwise returns
  // false.
  virtual bool sendReplayFinished();
  // Sends crash dump. Returns true if succeeded, otherwise returns false.
  virtual bool sendCrashDump(const std::string& filepath,
                             const void* crash_data, uint32_t crash_size);
  // Sends post data. Returns true if succeeded, otherwise returns false.
  virtual bool sendPostData(std::unique_ptr<Posts> posts);
  // Sends notification. Returns true if succeeded, otherwise returns false.
  virtual bool sendNotification(uint64_t id, uint32_t severity,
                                uint32_t api_index, uint64_t label,
                                const std::string& msg, const void* data,
                                uint32_t data_size);

 protected:
  ReplayConnection(ReplayGrpcStream* stream) : mGrpcStream(stream) {}

 private:
  // The gRPC stream connection.
  ReplayGrpcStream* mGrpcStream;
};
}  // namespace gapir

#endif  // GAPIR_REPLAY_CONNECTION_H
