#include "dns_portal.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "DNS_PORTAL";
static struct udp_pcb *s_pcb = NULL;
static ip_addr_t s_answer_ip;

typedef struct __attribute__((packed)) {
    uint16_t id, flags, qdcount, ancount, nscount, arcount;
} dns_hdr_t;

// Very small helpers to read/write name & A record; keeping it compact for brevity
static void udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (!p) return;
    // We’ll build a tiny response that mirrors the question ID and provides one A record pointing to s_answer_ip
    if (p->len < sizeof(dns_hdr_t)) { pbuf_free(p); return; }

    struct pbuf *q = pbuf_alloc(PBUF_TRANSPORT, p->len + 16 /*room for answer*/, PBUF_POOL);
    if (!q) { pbuf_free(p); return; }
    pbuf_take(q, p->payload, p->len);

    uint8_t *buf = q->payload;
    dns_hdr_t *hdr = (dns_hdr_t*)buf;
    hdr->flags   = PP_HTONS(0x8180); // standard response, no error
    hdr->ancount = PP_HTONS(1);

    // Skip question: header + qname + QTYPE(2) + QCLASS(2)
    // Walk qname until 0x00 (end of labels)
    int i = sizeof(dns_hdr_t);
    while (i < q->len && buf[i] != 0) i += (buf[i] + 1);
    if (i+5 >= q->len) { pbuf_free(q); pbuf_free(p); return; }
    i += 5; // skip 0x00, QTYPE, QCLASS

    // Append answer at position i
    // Name as pointer to offset 12 (0xC00C)
    uint8_t ans[16];
    int k = 0;
    ans[k++]=0xC0; ans[k++]=0x0C;  // name pointer
    ans[k++]=0x00; ans[k++]=0x01;  // type A
    ans[k++]=0x00; ans[k++]=0x01;  // class IN
    ans[k++]=0x00; ans[k++]=0x00; ans[k++]=0x00; ans[k++]=0x1E; // TTL 30s
    ans[k++]=0x00; ans[k++]=0x04;  // RDLENGTH 4
    // RDATA = IPv4 bytes (network order)
    ans[k++]=ip4_addr1(&s_answer_ip.u_addr.ip4);
    ans[k++]=ip4_addr2(&s_answer_ip.u_addr.ip4);
    ans[k++]=ip4_addr3(&s_answer_ip.u_addr.ip4);
    ans[k++]=ip4_addr4(&s_answer_ip.u_addr.ip4);

    // Extend buffer and copy answer
    pbuf_realloc(q, i + k);
    memcpy(((uint8_t*)q->payload) + i, ans, k);

    udp_sendto(pcb, q, addr, port);
    pbuf_free(q);
    pbuf_free(p);
}

esp_err_t dns_portal_start(uint32_t ap_ip)
{
    if (s_pcb) return ESP_OK;
    s_pcb = udp_new();
    if (!s_pcb) return ESP_ERR_NO_MEM;

    IP_ADDR4(&s_answer_ip, (ap_ip) & 0xFF, (ap_ip>>8)&0xFF, (ap_ip>>16)&0xFF, (ap_ip>>24)&0xFF);
    if (udp_bind(s_pcb, IP_ANY_TYPE, 53) != ERR_OK) {
        udp_remove(s_pcb); s_pcb = NULL; return ESP_FAIL;
    }
    udp_recv(s_pcb, udp_recv_cb, NULL);
    ESP_LOGI(TAG, "DNS captive portal started on :53");
    return ESP_OK;
}

void dns_portal_stop(void)
{
    if (s_pcb) { udp_remove(s_pcb); s_pcb = NULL; }
}
