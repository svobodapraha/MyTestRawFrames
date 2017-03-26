#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>    /* Must precede if*.h */
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>

union ethframe
{
  struct
  {
    struct ethhdr    header;
    unsigned char    data[ETH_DATA_LEN];
  } field;
  unsigned char    buffer[ETH_FRAME_LEN];
};

int main(int argc, char **argv) {
  char *iface = "lo";
  unsigned char dest[ETH_ALEN]
           = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  unsigned short proto = 0x1234;
  unsigned char *data = "hello world";
  unsigned short data_len = strlen(data);
 
  int s;
  if ((s = socket(AF_PACKET, SOCK_RAW, htons(proto))) < 0) {
    printf("Error: could not open socket\n");
    return -1;
  }
 
  struct ifreq buffer;
  int ifindex;
  memset(&buffer, 0x00, sizeof(buffer));
  strncpy(buffer.ifr_name, iface, IFNAMSIZ);
  if (ioctl(s, SIOCGIFINDEX, &buffer) < 0) {
    printf("Error: could not get interface index\n");
    close(s);
    return -1;
  }
  ifindex = buffer.ifr_ifindex;
 
  unsigned char source[ETH_ALEN];
  if (ioctl(s, SIOCGIFHWADDR, &buffer) < 0) {
    printf("Error: could not get interface address\n");
    close(s);
    return -1;
  }
  memcpy((void*)source, (void*)(buffer.ifr_hwaddr.sa_data),
         ETH_ALEN);
 
  union ethframe frame;
  memcpy(frame.field.header.h_dest, dest, ETH_ALEN);
  memcpy(frame.field.header.h_source, source, ETH_ALEN);
  frame.field.header.h_proto = htons(proto);
  memcpy(frame.field.data, data, data_len);
 
  unsigned int frame_len = data_len + ETH_HLEN;
 
  struct sockaddr_ll saddrll;
  memset((void*)&saddrll, 0, sizeof(saddrll));
  saddrll.sll_family = PF_PACKET;   
  saddrll.sll_ifindex = ifindex;
  saddrll.sll_halen = ETH_ALEN;
  memcpy((void*)(saddrll.sll_addr), (void*)dest, ETH_ALEN);
 
  if (sendto(s, frame.buffer, frame_len, 0,
             (struct sockaddr*)&saddrll, sizeof(saddrll)) > 0)
    printf("Success!\n");
  else
    printf("Error, could not send\n");

  //try to receive something...

  int recv_result;
  struct sockaddr_ll saddrll_rcv;
  socklen_t sll_len = sizeof(saddrll_rcv);
  char recv_buffer[ETH_FRAME_LEN];
  recv_result = recvfrom(s, recv_buffer, ETH_FRAME_LEN, 0,(struct sockaddr *)&saddrll_rcv, &sll_len);

     printf("Recv_result %u  %s\n", recv_result, recv_buffer+14);



  close(s);
  return 0;
}
