        AREA    |Asm$$Code|, CODE, READONLY

        EXPORT  copy_160_pixels
        EXPORT  copy_160_pixels_2x

copy_160_pixels
        STMFD   sp!,{r4-r11,lr}
        MOV     lr,#160
01
        SUBS    lr, lr, #40
        LDMIA   r1!,{r3-r12}
        STMIA   r0!,{r3-r12}
        BGT     %B01
        LDMFD   sp!,{r4-r11,pc}

copy_160_pixels_2x
        STMFD   sp!,{r4-r11,lr}
        MOV     lr, #160
01
        SUBS    lr, lr, #40
        LDMIA   r1!,{r3-r12}
        STMIA   r0!,{r3-r12}
        STMIA   r2!,{r3-r12}
        BGT     %B01
        LDMFD   sp!,{r4-r11,pc}

        END
