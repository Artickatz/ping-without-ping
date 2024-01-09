#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>

#define PACKET_SIZE 64
#define MAX_WAIT_TIME 5

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

void send_ping_request(int sock, struct sockaddr_in *dest_addr) {
    struct icmphdr icmp_header;
    char packet[PACKET_SIZE];

    // ICMP header
    icmp_header.type = ICMP_ECHO;
    icmp_header.code = 0;
    icmp_header.un.echo.id = getpid();
    icmp_header.un.echo.sequence = 0;
    icmp_header.checksum = 0;
    icmp_header.checksum = checksum(&icmp_header, sizeof(icmp_header));

    memcpy(packet, &icmp_header, sizeof(icmp_header));

    // Send the packet
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr)) == -1) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

void receive_ping_reply(int sock, struct sockaddr_in *dest_addr) {
    char buffer[PACKET_SIZE];
    ssize_t bytes_received;
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    // Receive the reply
    bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &sender_addr_len);

    if (bytes_received == -1) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    if (memcmp(&sender_addr, dest_addr, sizeof(struct sockaddr_in)) == 0) {
        printf("Device exists on the network.\n");
    } else {
        printf("Received a reply from an unexpected source.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <IP address>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &(dest_addr.sin_addr)) != 1) {
        perror("inet_pton");
        close(sock);
        return EXIT_FAILURE;
    }

    send_ping_request(sock, &dest_addr);

    // Wait for the reply
    sleep(MAX_WAIT_TIME);

    receive_ping_reply(sock, &dest_addr);

    close(sock);
    return EXIT_SUCCESS;
}
