#include "rpc-server.h"


int main(int argc, char* argv[])
{
    RpcServerImpl server;
    server.Run(55001);

    return 0;
}
