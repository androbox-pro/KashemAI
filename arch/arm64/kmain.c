#include <stdint.h>
#include <stddef.h>
#include "vipos.h"

extern const uint8_t vip_package_start[];
extern const uint8_t vip_package_end[];

/* QEMU 'virt' PL011 UART base address. Kept behind one constant so the future
 * device-tree/HAL work only needs to replace the platform mapping layer. */
#define UART0_BASE 0x09000000u
#define UARTDR (*(volatile uint32_t *)(UART0_BASE + 0x000u))
#define UARTRSR (*(volatile uint32_t *)(UART0_BASE + 0x004u))
#define UARTFR (*(volatile uint32_t *)(UART0_BASE + 0x018u))
#define UARTIBRD (*(volatile uint32_t *)(UART0_BASE + 0x024u))
#define UARTFBRD (*(volatile uint32_t *)(UART0_BASE + 0x028u))
#define UARTLCR_H (*(volatile uint32_t *)(UART0_BASE + 0x02Cu))
#define UARTCR (*(volatile uint32_t *)(UART0_BASE + 0x030u))
#define UARTIMSC (*(volatile uint32_t *)(UART0_BASE + 0x038u))
#define UARTICR (*(volatile uint32_t *)(UART0_BASE + 0x044u))

#define UARTFR_TXFF (1u << 5)
#define UARTFR_RXFE (1u << 4)

static void uart_init(void) {
    UARTCR = 0;
    UARTIMSC = 0;
    UARTICR = 0x7FF;
    UARTIBRD = 1;
    UARTFBRD = 40;
    UARTLCR_H = (3u << 5); /* 8N1, FIFO disabled for simplest bring-up */
    UARTCR = (1u << 9) | (1u << 8) | 1u; /* RXE | TXE | UARTEN */
}

static void uart_putc(char c) {
    while (UARTFR & UARTFR_TXFF) { }
    UARTDR = (uint32_t)(uint8_t)c;
}

static void uart_write(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

static char uart_getc(void) {
    while (UARTFR & UARTFR_RXFE) { }
    return (char)(UARTDR & 0xFFu);
}

static void uart_write_u64(uint64_t n) {
    char buf[21];
    size_t i = 0;
    if (n == 0) { uart_putc('0'); return; }
    while (n && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }
    while (i) uart_putc(buf[--i]);
}

static int str_eq(const char *a, const char *b) {
    size_t i = 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == '\0' && b[i] == '\0';
}

/* The firmware/QEMU gives us a DTB pointer in x0. For 0.2 we intentionally
 * only retain it as a diagnostic value; device-tree parsing will move into
 * the hardware abstraction layer in the next milestone. */
static uint64_t dtb_address;

static void show_help(void) {
    uart_write("Commands:\n");
    uart_write("  help   - show this help\n");
    uart_write("  about  - show VIPOS 0.2 ARM info\n");
    uart_write("  apps   - show bundled .vip package\n");
}

static void show_about(void) {
    uart_write("VIPOS Mobile Foundation\n");
    uart_write("Version: 0.2-dev\n");
    uart_write("Architecture: AArch64\n");
    uart_write("Board: QEMU virt\n");
    uart_write("Console: PL011 UART\n");
    uart_write("MMU: not enabled yet\n");
    uart_write("DTB address: ");
    uart_write_u64(dtb_address);
    uart_write("\n");
    uart_write("App format: .vip\n");
}

static const struct vip_header *vip_package(void) {
    return (const struct vip_header *)(uintptr_t)vip_package_start;
}

static int vip_package_valid(void) {
    const struct vip_header *h = vip_package();
    return h->magic[0] == VIP_MAGIC0 && h->magic[1] == VIP_MAGIC1 &&
           h->magic[2] == VIP_MAGIC2 && h->magic[3] == VIP_MAGIC3;
}

static void show_apps(void) {
    if (!vip_package_valid()) {
        uart_write("No valid .vip package mapped.\n");
        return;
    }
    const struct vip_header *h = vip_package();
    const char *m = (const char *)(uintptr_t)(vip_package_start + h->header_size);
    size_t i = 0;
    uart_write("Bundled VIP package:\n");
    while (i < h->manifest_size && (vip_package_start + h->header_size + i) < vip_package_end) {
        if ((i == 0 || m[i-1] == '\n')) {
            uart_write("  ");
            while (i < h->manifest_size && m[i] != '\n') uart_putc(m[i++]);
            uart_putc('\n');
        } else {
            ++i;
        }
    }
}

static void shell(void) {
    char line[64];
    uart_write("vipos-arm64> ");
    size_t n = 0;

    for (;;) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') {
            uart_write("\n");
            line[n] = '\0';
            if (str_eq(line, "help")) show_help();
            else if (str_eq(line, "about")) show_about();
            else if (str_eq(line, "apps")) show_apps();
            else if (n != 0) uart_write("Unknown command. Type 'help'.\n");
            n = 0;
            uart_write("vipos-arm64> ");
        } else if (c == '\b' || (uint8_t)c == 0x7Fu) {
            if (n > 0) { --n; uart_write("\b \b"); }
        } else if (n + 1 < sizeof(line)) {
            line[n++] = c;
            uart_putc(c);
        }
    }
}

void arm64_kmain(uint64_t dtb) {
    dtb_address = dtb;
    uart_init();
    uart_write("VIPOS_KERNEL_OK\n");
    uart_write("VIPOS_ARM64_OK\n");
    uart_write("VIPOS Mobile Foundation booted on AArch64.\n");
    shell();
}
