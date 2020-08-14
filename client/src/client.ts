import * as protoLoader from "@grpc/proto-loader";
import * as grpc from "grpc";
import * as path from "path";

grpc.setLogger(console);
grpc.setLogVerbosity(grpc.logVerbosity.DEBUG);

// TODO: load env file for env
const PORT = process.env.PORT ?? 55001;
const PROTO_PATH = path.resolve(__dirname, "service.proto");

const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
  keepCase: true,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true,
});

const packageDef = grpc.loadPackageDefinition(packageDefinition)
  .mvr as grpc.GrpcObject;

const Client = (packageDef.Service as any) as new (...a: any[]) => grpc.Client;

const makeClient = () =>
  new Client(
    `localhost:${PORT}`,
    grpc.credentials.createInsecure(),
    // SEE: https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
    {
      //"grpc.max_concurrent_streams": 10,
      //"grpc.max_receive_message_length": 100 * 1024 * 1024,
      "grpc.max_receive_message_length": 100 * 1024 * 1024,
      "grpc.max_send_message_length": 100 * 1024 * 10241,
      "grpc.http2.max_frame_size": 16777215,
      "grpc.http2.write_buffer_size": 67108864, // grpc max
      //"grpc.experimental.tcp_max_read_chunk_size": 200 * 1024 * 1024,
      "grpc.loadreporting": 1,
      "grpc.client_idle_timeout_ms": 4000,
      "grpc.http2.hpack_table_size.decoder": 100 * 1024 * 1024,
      "grpc.http2.hpack_table_size.encoder": 100 * 1024 * 1024,
    }
  );

const syncClient = makeClient() as any;

export default syncClient;
