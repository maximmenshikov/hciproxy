#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/* Open UDP socket that would receive requests and transmit responses */
static int
srv_open(int port)
{
    struct sockaddr_in server_addr;
    int                sk;

    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (sk == -1)
        return -1;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sk, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) != 0)
    {
        fprintf(stderr, "bind(udp): %s\n", strerror(errno));
        close(sk);
        exit(EXIT_FAILURE);
    }

    if (fcntl(sk, F_SETFL, O_NONBLOCK) == -1)
    {
        fprintf(stderr, "fcntl(udp): %s\n", strerror(errno));
        close(sk);
        exit(EXIT_FAILURE);
    }

    return sk;
}

int
_hci_open_dev(int dev)
{
    struct sockaddr_hci addr;
    int                 sk;

    sk = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
    if (sk < 0)
        return sk;

    memset(&addr, 0, sizeof(addr));
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = dev;
    addr.hci_channel = HCI_CHANNEL_USER;
    if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "bind(hci): %s\n", strerror(errno));
        close(sk);
        exit(EXIT_FAILURE);
    }

    if (fcntl(sk, F_SETFL, O_NONBLOCK) == -1)
    {
        fprintf(stderr, "fcntl(hci): %s\n", strerror(errno));
        close(sk);
        exit(EXIT_FAILURE);
    }

    return sk;
}

static void
print_buf(const char *b, size_t len)
{
    size_t i;

    for (i = 0; i < len; ++i)
    {
        if ((i % 8) == 0 && i > 0)
        {
            fprintf(stderr, "\n      ");
        }
        fprintf(stderr, "%02X ", b[i]);
    }
    fprintf(stderr, "\n");
}

/* Simple input/output monitor */
static void *
monitor_cb(int srv, int hci)
{
    struct sockaddr_in addr;
    int                addr_valid = 0;
    socklen_t          clientlen = sizeof(addr);

    while (1)
    {
        unsigned char buf[HCI_MAX_EVENT_SIZE];
        int           ret;

        ret = recvfrom(srv, buf, sizeof(buf), MSG_DONTWAIT,
                       (struct sockaddr *)&addr, &clientlen);
        if (ret > 0)
        {
            int found = 0;
            fprintf(stderr, "< Peer: ");
            print_buf(buf, ret);
            fprintf(stderr, " - total %d bytes\n", ret);
            addr_valid = 1;

            if (found == 0)
                write(hci, buf, ret);
        }

        ret = read(hci, buf, sizeof(buf));
        if (ret > 0)
        {
            fprintf(stderr, "> HCI:  ");
            print_buf(buf, ret);
            fprintf(stderr, " - total %d bytes\n", ret);
            if (addr_valid == 1)
            {
                sendto(srv, buf, ret, 0,
                       (struct sockaddr *)&addr, clientlen);
            }
        }
    }
}

int main(int argc, const char *argv[])
{
    int arg;
    int udp_port = 5712;
    int dev = 0;

    for (arg = 1; arg < argc; ++arg)
    {
        if (strcmp(argv[arg], "--port") == 0 && (arg + 1) < argc)
        {
            udp_port = atoi(argv[arg + 1]);
            arg++;
        }
        if (strcmp(argv[arg], "--dev") == 0 && (arg + 1) < argc)
        {
            dev = atoi(argv[arg + 1]);
            arg++;
        }
    }

    monitor_cb(srv_open(udp_port), _hci_open_dev(dev));
    return 0;
}
