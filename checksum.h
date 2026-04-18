#ifndef CHECKSUM_H
#define CHECKSUM_H


inline unsigned short checksum_add (
    unsigned short *buf, int len, int noFlip) {
  unsigned long long cksum = 0;

  for (; len > 1; len -= 2) {
    cksum += le16toh(*buf);
    buf++;
  }
  if (len) {
    cksum += *(unsigned char *) buf;
  }
  cksum = (cksum >> 16) + (cksum & 0xFFFF);
  cksum = (cksum >> 16) + (cksum & 0xFFFF);

  if (!noFlip) {
    cksum = ~cksum;
  }
  return cksum;
}


inline unsigned int checksum_64K (void *buf, int len) {
  unsigned int cksum = 0;

  for (int i = 0; i < len >> 2; i++) {
    cksum += le32toh(*(unsigned int *) buf);
    buf = (char *) buf + 4;
  }
  switch (len & 3) {
    case 1:
      cksum += *(unsigned char *) buf;
      break;
    case 2:
      cksum += le16toh(*(unsigned short *) buf);
      break;
    case 3:
      cksum += le32toh(*(unsigned int *) buf) & 0xFFFFFF;
      break;
  }

  return cksum;
}


inline unsigned short originale_add (unsigned short *buf, int len) {
  return checksum_add(buf, len, 1);
}


inline unsigned short checksum (unsigned short *buf, int len) {
  return checksum_add(buf, len, 0);
}


#endif  // CHECKSUM_H
