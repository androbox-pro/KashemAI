#include <stdint.h>
#include <stddef.h>
#include "vipos.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static uint8_t color = 0x07;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}


static void debug_putc(char c) {
    outb(0x00E9, (uint8_t)c);
}

static void debug_write(const char *s) {
    while (*s) debug_putc(*s++);
}

#define COM1 0x3F8

static void serial_init(void) {
    outb(COM1 + 1, 0x00); // Disable interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x03); // 38400/3 = 38400 baud divisor
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); // 8N1
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear queues
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20u) == 0) { }
    outb(COM1, (uint8_t)c);
}

static void serial_write(const char *s) {
    while (*s) serial_putc(*s++);
}

static void console_move_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = (uint16_t)y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void console_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        VGA_MEMORY[i] = ((uint16_t)color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    console_move_cursor(cursor_x, cursor_y);
}

static void console_scroll(void) {
    if (cursor_y < VGA_HEIGHT) return;
    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ((uint16_t)color << 8) | ' ';
    }
    cursor_y = VGA_HEIGHT - 1;
}

static void console_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        ++cursor_y;
        console_scroll();
        console_move_cursor(cursor_x, cursor_y);
        return;
    }
    if (c == '\r') return;
    if (c == '\b') {
        if (cursor_x > 0) {
            --cursor_x;
            VGA_MEMORY[(size_t)cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | ' ';
        }
        console_move_cursor(cursor_x, cursor_y);
        return;
    }
    VGA_MEMORY[(size_t)cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | (uint8_t)c;
    ++cursor_x;
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        ++cursor_y;
        console_scroll();
    }
    console_move_cursor(cursor_x, cursor_y);
}

static void console_write(const char *s) {
    while (*s) console_putc(*s++);
}

static void console_write_u32(uint32_t n) {
    char buf[11];
    size_t i = 0;
    if (n == 0) { console_putc('0'); return; }
    while (n && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i) console_putc(buf[--i]);
}

static const char keymap[128] = {
    [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',[0x06]='5',[0x07]='6',[0x08]='7',[0x09]='8',[0x0A]='9',[0x0B]='0',
    [0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',[0x14]='t',[0x15]='y',[0x16]='u',[0x17]='i',[0x18]='o',[0x19]='p',
    [0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',[0x22]='g',[0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',
    [0x2C]='z',[0x2D]='x',[0x2E]='c',[0x2F]='v',[0x30]='b',[0x31]='n',[0x32]='m',
    [0x39]=' '
};

static char keyboard_read_char(void) {
    for (;;) {
        if ((inb(0x64) & 1u) == 0) continue;
        uint8_t sc = inb(0x60);
        if (sc & 0x80u) continue; // key release
        if (sc == 0x1C) return '\n';
        if (sc == 0x0E) return '\b';
        if (sc < 128 && keymap[sc]) return keymap[sc];
    }
}

static size_t str_len(const char *s) {
    size_t n = 0;
    while (s[n]) ++n;
    return n;
}

static int str_eq(const char *a, const char *b) {
    size_t i = 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == '\0' && b[i] == '\0';
}

static void vip_info(void);
static void vip_apps(void);
static void vip_run_demo(void);

static const uint8_t* vip_payload(uint32_t *size_out) {
    const struct vip_header *h = (const struct vip_header*)VIP_PACKAGE_ADDR;
    if (h->magic[0] != VIP_MAGIC0 || h->magic[1] != VIP_MAGIC1 ||
        h->magic[2] != VIP_MAGIC2 || h->magic[3] != VIP_MAGIC3) {
        *size_out = 0;
        return NULL;
    }
    *size_out = h->payload_size;
    return (const uint8_t*)(VIP_PACKAGE_ADDR + h->header_size + h->manifest_size);
}

static void manifest_value(const char *key, char *out, size_t out_cap) {
    const struct vip_header *h = (const struct vip_header*)VIP_PACKAGE_ADDR;
    const char *m = (const char*)(VIP_PACKAGE_ADDR + h->header_size);
    size_t key_len = str_len(key);
    size_t i = 0;
    if (out_cap == 0) return;
    out[0] = '\0';

    while (i < h->manifest_size) {
        if ((i == 0 || m[i-1] == '\n') && i + key_len < h->manifest_size) {
            size_t j = 0;
            while (j < key_len && m[i+j] == key[j]) ++j;
            if (j == key_len && i + key_len < h->manifest_size && m[i+key_len] == '=') {
                size_t p = i + key_len + 1;
                size_t o = 0;
                while (p < h->manifest_size && m[p] != '\n' && o + 1 < out_cap) out[o++] = m[p++];
                out[o] = '\0';
                return;
            }
        }
        ++i;
    }
}

static void vip_apps(void) {
    char name[64];
    char version[24];
    manifest_value("name", name, sizeof(name));
    manifest_value("version", version, sizeof(version));
    console_write("Installed VIP application:\n  ");
    console_write(name[0] ? name : "Unknown");
    console_write(" v");
    console_write(version[0] ? version : "?");
    console_putc('\n');
}

static void vip_info(void) {
    const struct vip_header *h = (const struct vip_header*)VIP_PACKAGE_ADDR;
    if (h->magic[0] != VIP_MAGIC0 || h->magic[1] != VIP_MAGIC1 ||
        h->magic[2] != VIP_MAGIC2 || h->magic[3] != VIP_MAGIC3) {
        console_write("No valid .vip package loaded.\n");
        return;
    }
    char name[64], id[64], version[24];
    manifest_value("name", name, sizeof(name));
    manifest_value("id", id, sizeof(id));
    manifest_value("version", version, sizeof(version));
    console_write("VIP package information\n");
    console_write("  Name: "); console_write(name); console_putc('\n');
    console_write("  ID: "); console_write(id); console_putc('\n');
    console_write("  Version: "); console_write(version); console_putc('\n');
    console_write("  Package version: "); console_write_u32(h->version); console_putc('\n');
    console_write("  Payload bytes: "); console_write_u32(h->payload_size); console_putc('\n');
    console_write("  Payload type: "); console_write_u32(h->payload_type); console_putc('\n');
}

static void vip_run_demo(void) {
    uint32_t size = 0;
    const uint8_t *p = vip_payload(&size);
    if (!p || size == 0) {
        console_write("Cannot run: invalid .vip package.\n");
        return;
    }
    console_write("[VIP Runtime] launching app...\n");
    uint32_t pc = 0;
    while (pc < size) {
        uint8_t op = p[pc++];
        if (op == 0x01) { // PRINT
            if (pc >= size) { console_write("VM error: truncated PRINT.\n"); return; }
            uint8_t len = p[pc++];
            if (pc + len > size) { console_write("VM error: bad length.\n"); return; }
            for (uint8_t i = 0; i < len; ++i) console_putc((char)p[pc++]);
        } else if (op == 0x02) { // NEWLINE
            console_putc('\n');
        } else if (op == 0x03) { // CLEAR
            console_clear();
        } else if (op == 0xFF) { // HALT
            break;
        } else {
            console_write("VM error: unknown opcode.\n");
            return;
        }
    }
    console_write("\n[VIP Runtime] app exited.\n");
}

static void reboot(void) {
    console_write("Rebooting...\n");
    for (volatile uint32_t i = 0; i < 1000000u; ++i) { }
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile ("hlt");
}

static void show_help(void) {
    console_write("Commands:\n");
    console_write("  help      - show this help\n");
    console_write("  about     - show VIPOS info\n");
    console_write("  apps      - list .vip apps\n");
    console_write("  vipinfo   - inspect demo .vip\n");
    console_write("  run       - run demo .vip app\n");
    console_write("  clear     - clear screen\n");
    console_write("  reboot    - reboot machine\n");
}

static void show_about(void) {
    console_write("VIPOS Developer Edition\n");
    console_write("Version: 0.1 prototype\n");
    console_write("Kernel: freestanding 32-bit protected mode\n");
    console_write("App format: .vip\n");
    console_write("Runtime: VIP Bytecode v1\n");
    console_write("Target: x86 BIOS prototype; mobile port comes later\n");
}

static void shell(void) {
    char line[80];
    for (;;) {
        console_write("vipos> ");
        size_t n = 0;
        for (;;) {
            char c = keyboard_read_char();
            if (c == '\n') {
                console_putc('\n');
                break;
            }
            if (c == '\b') {
                if (n > 0) {
                    --n;
                    console_putc('\b');
                }
                continue;
            }
            if (n + 1 < sizeof(line)) {
                line[n++] = c;
                console_putc(c);
            }
        }
        line[n] = '\0';
        if (str_eq(line, "help")) show_help();
        else if (str_eq(line, "about")) show_about();
        else if (str_eq(line, "apps")) vip_apps();
        else if (str_eq(line, "vipinfo")) vip_info();
        else if (str_eq(line, "run")) vip_run_demo();
        else if (str_eq(line, "clear")) console_clear();
        else if (str_eq(line, "reboot")) reboot();
        else if (n != 0) {
            console_write("Unknown command. Type 'help'.\n");
        }
    }
}

void kmain(void) {
    debug_write("VIPOS_KERNEL_OK\n");
    serial_init();
    serial_write("VIPOS_KERNEL_OK\n");
    console_clear();
    console_write("========================================\n");
    console_write("           VIPOS 0.1 PROTOTYPE          \n");
    console_write("========================================\n");
    console_write("Welcome to VIPOS!\n");
    console_write("Type 'help' for commands.\n\n");
    shell();
}
