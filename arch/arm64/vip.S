.section .rodata.vip,"a",@progbits
.global vip_package_start
.global vip_package_end
vip_package_start:
    .incbin "build/demo.vip"
vip_package_end:
