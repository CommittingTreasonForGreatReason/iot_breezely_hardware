#include <Arduino.h>
#include "utils.hpp"

void text_input_blocking(char *input_buffer, uint8_t size)
{
    bool confirmed_input = false;
    uint8_t text_input_index = 0;
    while (!confirmed_input)
    {
        if (Serial.available() > 0)
        {
            // read the incoming byte:
            uint8_t read_byte = Serial.read();

            if (text_input_index + 1 >= size)
            {
                Serial.print("input too long");
            }
            else
            {
                if (read_byte == DELETE_ASCII)
                {
                    text_input_index--;
                    input_buffer[text_input_index] = 0x00;
                }
                else if (read_byte == ENTER_ASCII)
                {
                    input_buffer[text_input_index + 1] = 0x00;
                    confirmed_input = true;
                    Serial.println("");
                }
                else
                {
                    Serial.print((char)read_byte);
                    input_buffer[text_input_index] = read_byte;
                    text_input_index++;
                }
            }
        }
    }
}

uint8_t dot_dot_dot_index = 0;
void dot_dot_dot_loop_increment()
{
    dot_dot_dot_index++;
    if (dot_dot_dot_index >= 3)
    {
        dot_dot_dot_index = 0;
        Serial.println("\r");
    }
    Serial.print(".");
}