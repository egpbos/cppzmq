#include <catch.hpp>
#include <zmq.hpp>

#include <cerrno>
#include <unistd.h>
#include <csignal> // kill
//#include <wait.h> // waitpid

TEST_CASE("context construct default and destroy", "[context]")
{
    zmq::context_t context;
}

TEST_CASE("context create, close and destroy", "[context]")
{
    zmq::context_t context;
    context.close();
    CHECK(NULL == (void *) context);
}

TEST_CASE("context close, dealing with EINTR", "[context]")
{
    for (int i = 0; i < 100; ++i) {
        pid_t child_pid = fork();
        if (child_pid == 0) {
//            kill(getpid(), SIGTRAP);
            // in child process
            zmq::context_t context;
            zmq::socket_t socket(context, ZMQ_PAIR);
            socket.connect("ipc:///tmp/test_context_close_eintr.ipc");

            zmq::message_t reply (5);
            memcpy (reply.data (), "World", 5);
            socket.send (reply);

            socket.close();
//            context.close();
            _exit(0);
        } else {
            // in parent process
            zmq::context_t context;
            zmq::socket_t socket(context, ZMQ_PAIR);
            socket.bind("ipc:///tmp/test_context_close_eintr.ipc");

            zmq::message_t request;
            socket.recv(&request);

//            waitpid(child_pid, nullptr, WNOHANG); // don't handle 0 (== child status unchanged)
            socket.close();
            context.close();
//            CHECK(context.close() != -1);
//            CHECK(zmq::error_t().num() != 4);
        }
    }
}


//TEST_CASE("context close, dealing with EINTR", "[context]")
//{
//  zmq::context_t context;
//  zmq::socket_t sender(context, ZMQ_PAIR);
//  context.close();
////    errno = EINTR;
//}
