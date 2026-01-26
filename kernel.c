// kernel.c
void kmain(void) {
    const char *str = "Welcome to MonkeyOS! Hardware is hard.";
    char *vidptr = (char*)0xb8000; // Video memory begins here
    unsigned int i = 0;
    unsigned int j = 0;

    // Clear the screen (fill with spaces)
    while(j < 80 * 25 * 2) {
        vidptr[j] = ' ';
        vidptr[j+1] = 0x07; // Light grey text on black background
        j = j + 2;
    }

    j = 0;
    // Write the string to video memory
    while(str[i] != '\0') {
        vidptr[j] = str[i];     // The character
        vidptr[j+1] = 0x07;     // The color attribute
        i++;
        j = j + 2;
    }
    return;
}