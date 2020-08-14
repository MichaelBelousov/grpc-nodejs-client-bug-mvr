#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

#include <grpcpp/grpcpp.h>

#include "service.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

class RpcServerImpl final {
	std::unique_ptr<ServerCompletionQueue> cq_;
	Mvr::MyService::AsyncService service_;
	std::unique_ptr<Server> server_;
public:
	~RpcServerImpl() {
		server_->Shutdown();
		// Always shutdown the completion queue after the server.
		cq_->Shutdown();
	}

	// There is no shutdown handling in this code.
	void Run(unsigned short port) {
		std::string server_address = "0.0.0.0:" + std::to_string(port);

		ServerBuilder builder;
		// Listen on the given address without any authentication mechanism.
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		// Register "service_" as the instance through which we'll communicate with
		// clients. In this case it corresponds to an *asynchronous* service.
		builder.RegisterService(&service_);
		// Get hold of the completion queue used for the asynchronous communication
		// with the gRPC runtime.
		// TODO: expose via CLI
		//builder.SetMaxMessageSize(100 * 1024 * 1024);
		builder.SetMaxReceiveMessageSize(50 * 1024 * 1024);
		builder.AddChannelArgument("grpc.http2.max_frame_size", 16777215);
		builder.AddChannelArgument("grpc.http2.write_buffer_size", 67108864);
		cq_ = builder.AddCompletionQueue();
		// Finally assemble the server.
		server_ = builder.BuildAndStart();
		std::cout << "Server listening on " << server_address << std::endl;

		MainLoop();
	}

private:
	class ICallData {
	public:
		virtual ~ICallData() {}
		virtual void Proceed() = 0;
	};
	template<typename TRequest, typename TReply = Mvr::Empty>
	class CallData : public ICallData {
	public:
		CallData(Mvr::MyService::AsyncService* service, ServerCompletionQueue* cq)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CallStatus::Create)
		{}

		virtual grpc::Status DoHandle() = 0;
		virtual void New() = 0; // EEK: rename to Pump or something?
		void Proceed() {
			if (status_ == CallStatus::Process) {
				// Spawn a new CallData instance to serve new clients while we process
				// the one for this CallData. The instance will deallocate itself as
				// part of its FINISH state.
				New();

				// run the intended operation
				Status Result = DoHandle();

				// And we are done! Let the gRPC runtime know we've finished, using the
				// memory address of this instance as the uniquely identifying tag for
				// the event.
				status_ = CallStatus::Finish;
				responder_.Finish(reply_, Result, this);
			}
			else {
				GPR_ASSERT(status_ == CallStatus::Finish);
				// Once in the FINISH state, deallocate ourselves (CallData).
				delete this;
			}
		}

	protected:
		// The means of communication with the gRPC runtime for an asynchronous
		// server.
		Mvr::MyService::AsyncService* service_;
		// The producer-consumer queue where for asynchronous server notifications.
		ServerCompletionQueue* cq_;
		// Context for the rpc, allowing to tweak aspects of it such as the use
		// of compression, authentication, as well as to send metadata back to the
		// client.
		ServerContext ctx_;

		// What we get from the client.
		TRequest request_;
		// What we send back to the client.
		TReply reply_;

		// The means to get back to the client.
		ServerAsyncResponseWriter<Mvr::Empty> responder_;

		// Let's implement a tiny state machine with the following states.
		enum class CallStatus { Create, Process, Finish };
		CallStatus status_;  // The current serving state.
	};

	class SayACallData : public CallData<Mvr::MessageA> {
		grpc::Status DoHandle() override {
			std::cout << "Type: SayA" << std::endl;
			std::cout << "Scene Name: " << request_.name() << std::endl;
			return grpc::Status::OK;
		}
		void New() override {
			new SayACallData(service_, cq_);
		}
	public:
		template<typename ...SuperArgs>
		SayACallData(SuperArgs&& ...superArgs) : CallData<Mvr::MessageA>(std::forward<SuperArgs>(superArgs)...) {
			if (status_ == CallStatus::Create) {
				status_ = CallStatus::Process;
				service_->RequestSayA(&ctx_, &request_, &responder_, cq_, cq_, this);
			}
		}
	};

	class SayBCallData : public CallData<Mvr::MessageB> {
		grpc::Status DoHandle() override {
			std::cout << "Type: SayB" << std::endl;
			std::cout << "Scene Name: " << request_.name() << std::endl;
			return grpc::Status::OK;
		}
		void New() override {
			new SayBCallData(service_, cq_);
		}
	public:
		template<typename ...SuperArgs>
		SayBCallData(SuperArgs&& ...superArgs) : CallData<Mvr::MessageB>(std::forward<SuperArgs>(superArgs)...) {
			if (status_ == CallStatus::Create) {
				status_ = CallStatus::Process;
				service_->RequestSayB(&ctx_, &request_, &responder_, cq_, cq_, this);
			}
		}
	};

	void MainLoop() {
		// XXX: need to delete the remaining CallDatas at the end
		auto AddMeshCallData = new SayACallData(&service_, cq_.get());
		auto CreateSceneCallData = new SayBCallData(&service_, cq_.get());
		ICallData* tag;
		bool ok;
		while (true) {
			GPR_ASSERT(cq_->Next((void**)&tag, &ok));
			GPR_ASSERT(ok);
			tag->Proceed();
		}
	}
};
