#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>



#define TAG_B_PLUS 205
#define TAG_B_MINUS 206

#define TAG_C_PLUS 207
#define TAG_C_MINUS 208

#define TAG_D_PLUS 209
#define TAG_D_MINUS 210

#define PIN_ANALOG_CYL_1 23
#define PIN_ANALOG_CYL_2 22

#define PIN_A_VALVE 14
#define PIN_B_VALVE 15
#define PIN_C_VALVE 16
#define PIN_D_VALVE 17


#define CYL_INIT 0
#define CYL_FILLED 1
#define CYL_EMPTYING 2
#define CYL_EMPTY 3
#define CYL_FILLING 4


static uint16_t value = 15000;      // every widget is hooked to this value
static char message[41];            // 40 character text entry field
static uint16_t options = OPT_FLAT;
static byte prevkey;


static int16_t a_percent = 60, b_percent = 20, c_percent = 15, d_percent = 5;


static int16_t cy_1_adc_full = 0, cy_2_adc_full = 0;
static int16_t cy_1_adc_empty = 4096, cy_2_adc_empty = 4096;

static int16_t cy_1_pos = 0, cy_2_pos = 0;


static int16_t a_valve = 0, b_valve = 0, c_valve = 0, d_valve = 0;



void setup()
{
  memset(message, 7, 40);
  GD.begin(~GD_STORAGE);

  analogReadResolution(12);
  analogReadAveraging(16);

  Serial.begin(9600);

  pinMode(PIN_A_VALVE, OUTPUT);
  pinMode(PIN_B_VALVE, OUTPUT);
  pinMode(PIN_C_VALVE, OUTPUT);
  pinMode(PIN_D_VALVE, OUTPUT);
  
}




void loop()
{

   
   get_cyl_pos();

   cyl_1_state_machine();
   cyl_2_state_machine();

   
   digitalWrite(PIN_A_VALVE, a_valve);
   digitalWrite(PIN_B_VALVE, b_valve);
   digitalWrite(PIN_C_VALVE, c_valve);
   digitalWrite(PIN_D_VALVE, d_valve);



  
  GD.get_inputs();
  switch (GD.inputs.tag)
  {
        case TAG_B_PLUS:
            if (prevkey != TAG_B_PLUS )
              {
               adjust_b(5);
              }
         break;
    
        case TAG_B_MINUS:
            if (prevkey != TAG_B_MINUS )
              {
               adjust_b(-5);
              }
         break;
         
        case TAG_C_PLUS:
            if (prevkey != TAG_C_PLUS )
              {
               adjust_c(5);
              }
         break;
    
        case TAG_C_MINUS:
            if (prevkey != TAG_C_MINUS )
              {
               adjust_c(-5);
              }
         break;
    
        case TAG_D_PLUS:
            if (prevkey != TAG_D_PLUS )
              {
               adjust_d(5);
              }
         break;
    
        case TAG_D_MINUS:
            if (prevkey != TAG_D_MINUS )
              {
               adjust_d(-5);
              }
         break;
  }
  
  prevkey = GD.inputs.tag;
  
  
  GD.cmd_gradient(0, 0,   0x404044, 480, 480, 0x606068);
  GD.ColorRGB(0x707070);

  GD.LineWidth(4 * 16);
  GD.Begin(RECTS);

  GD.Vertex2ii(8, 8);
  GD.Vertex2ii(180, 264);


  GD.ColorRGB(0xffffff);

  GD.cmd_text(70,16, 24, 0, "%");
  GD.cmd_text(12, 40, 30, 0, "A");
  GD.cmd_text(12, 100, 30, 0, "B");
  GD.cmd_text(12, 160, 30, 0, "C");
  GD.cmd_text(12, 220, 30, 0, "D");

  GD.cmd_number(40, 40, 30, 0, a_percent);
  GD.cmd_number(40, 100, 30, 0, b_percent);
  GD.cmd_number(40, 160, 30, 0, c_percent);
  GD.cmd_number(40, 220, 30, 0, d_percent);
  
  GD.Tag(TAG_B_MINUS);
  GD.cmd_button(100, 100, 30, 30, 28, options, "-");
  GD.Tag(TAG_B_PLUS);
  GD.cmd_button(135, 100, 30, 30, 28, options, "+");

  GD.Tag(TAG_C_MINUS);
  GD.cmd_button(100, 160, 30, 30, 28, OPT_FLAT, "-");
  GD.Tag(TAG_C_PLUS);
  GD.cmd_button(135, 160, 30, 30, 28, OPT_FLAT, "+");

  GD.Tag(TAG_D_MINUS);
  GD.cmd_button(100, 220, 30, 30, 28, OPT_FLAT, "-");
  GD.Tag(TAG_D_PLUS);
  GD.cmd_button(135, 220, 30, 30, 28, OPT_FLAT, "+");
  
 
 
  GD.Tag(255);
  GD.cmd_progress(220, 20, 40, 180, options, (1000 - cy_1_pos), 1000);
  GD.cmd_progress(300, 20, 40, 180, options, (1000 - cy_2_pos), 1000);

  if(a_valve == 1)
    {
      GD.cmd_text(260, 220, 30, 0, "A");
    }

  if(b_valve == 1)
    {
      GD.cmd_text(260, 220, 30, 0, "B");
    }

  if(c_valve == 1)
    {
      GD.cmd_text(260, 220, 30, 0, "C");
    }

  if(d_valve == 1)
    {
      GD.cmd_text(260, 220, 30, 0, "D");
    }

  GD.swap();
}






void cyl_1_state_machine()
{

   static int16_t cy_1_state = 0;
  
   if(cy_1_state == CYL_INIT && cy_1_adc_full - cy_1_adc_empty <= 200)
    {
      cy_1_pos = 0;
    }
   
   if(cy_1_state == CYL_INIT && cy_1_adc_full - cy_1_adc_empty > 200 && cy_1_pos < 4)
    {
     cy_1_state = CYL_EMPTY;
    }
   
   if(( cy_1_state == CYL_EMPTYING) && cy_1_pos < 4)
    {
     cy_1_state = CYL_EMPTY;
    }


   if(cy_1_state == CYL_EMPTY && cy_1_pos > 4)
    {
      cy_1_state = CYL_FILLING;
    }

   if( cy_1_state == CYL_FILLING)
    {
      if( cy_1_pos < (a_percent*10))
        {
          a_valve = 1;
          b_valve = 0;
          c_valve = 0;
          d_valve = 0;
        }
      else if (cy_1_pos >= (a_percent*10) && cy_1_pos < (b_percent + a_percent)*10)
        {
          a_valve = 0;
          b_valve = 1;
          c_valve = 0;
          d_valve = 0;
        }
      else if (cy_1_pos >= (a_percent + b_percent)*10 && cy_1_pos < (c_percent + b_percent + a_percent)*10)
        {
          a_valve = 0;
          b_valve = 0;
          c_valve = 1;
          d_valve = 0;
        }

       else if (cy_1_pos >= (a_percent + b_percent + c_percent)*10 && cy_1_pos < 1000)
        {
          a_valve = 0;
          b_valve = 0;
          c_valve = 0;
          d_valve = 1;
        }
    }


   if(cy_1_state == CYL_FILLING && cy_1_pos > 996)
    {
     cy_1_state = CYL_FILLED;
     a_valve = 0;
     b_valve = 0;
     c_valve = 0;
     d_valve = 0;
     
    }

   if(cy_1_state == CYL_FILLED && cy_1_pos < 994)
    {
     cy_1_state = CYL_EMPTYING;
     a_valve = 0;
     b_valve = 0;
     c_valve = 0;
     d_valve = 0;
      
    }

}





void cyl_2_state_machine()
{
  static int16_t  cy_2_state = 0;
   if(cy_2_state == CYL_INIT && cy_2_adc_full - cy_2_adc_empty <= 200)
    {
      cy_2_pos = 0;
    }
   
   if(cy_2_state == CYL_INIT && cy_2_adc_full - cy_2_adc_empty > 200 && cy_2_pos < 4)
    {
     cy_2_state = CYL_EMPTY;
    }
   
   if(( cy_2_state == CYL_EMPTYING) && cy_2_pos < 4)
    {
     cy_2_state = CYL_EMPTY;
    }


   if(cy_2_state == CYL_EMPTY && cy_2_pos > 4)
    {
      cy_2_state = CYL_FILLING;
    }

   if( cy_2_state == CYL_FILLING)
    {
      if( cy_2_pos < (a_percent*10))
        {
          a_valve = 1;
          b_valve = 0;
          c_valve = 0;
          d_valve = 0;
        }
      else if (cy_2_pos >= (a_percent*10) && cy_2_pos < (b_percent + a_percent)*10)
        {
          a_valve = 0;
          b_valve = 1;
          c_valve = 0;
          d_valve = 0;
        }
      else if (cy_2_pos >= (a_percent + b_percent)*10 && cy_2_pos < (c_percent + b_percent + a_percent)*10)
        {
          a_valve = 0;
          b_valve = 0;
          c_valve = 1;
          d_valve = 0;
        }

       else if (cy_2_pos >= (a_percent + b_percent + c_percent)*10 && cy_2_pos < 1000)
        {
          a_valve = 0;
          b_valve = 0;
          c_valve = 0;
          d_valve = 1;
        }
    }


   if(cy_2_state == CYL_FILLING && cy_2_pos > 996)
    {
     cy_2_state = CYL_FILLED;
     a_valve = 0;
     b_valve = 0;
     c_valve = 0;
     d_valve = 0;
     
    }

   if(cy_2_state == CYL_FILLED && cy_2_pos < 994)
    {
     cy_2_state = CYL_EMPTYING;
     a_valve = 0;
     b_valve = 0;
     c_valve = 0;
     d_valve = 0;
      
    }

}






void get_cyl_pos()
{
  int16_t cy_1_raw = analogRead(PIN_ANALOG_CYL_1);
  int16_t cy_2_raw = analogRead(PIN_ANALOG_CYL_2);

  if (cy_1_raw > cy_1_adc_full + 2)
    {
      cy_1_adc_full = cy_1_raw;
    }
  
  if (cy_2_raw > cy_2_adc_full + 2)
    {
      cy_2_adc_full = cy_2_raw;
    }
  
  if (cy_1_raw < cy_1_adc_empty - 2)
    {
      cy_1_adc_empty = cy_1_raw;
    }
  
  if (cy_2_raw < cy_2_adc_empty - 2)
    {
      cy_2_adc_empty = cy_2_raw;
    }
    

   cy_1_pos = 1000.0*((float)(cy_1_raw - cy_1_adc_empty) / (float)(cy_1_adc_full - cy_1_adc_empty));
   cy_2_pos = 1000.0*((float)(cy_2_raw - cy_2_adc_empty) / (float)(cy_2_adc_full - cy_2_adc_empty));

   cy_1_pos = (cy_1_pos > 1000) ? 1000 : cy_1_pos;
   cy_1_pos = (cy_1_pos < 0) ? 0 : cy_1_pos;
   cy_2_pos = (cy_2_pos > 1000) ? 1000 : cy_2_pos;
   cy_2_pos = (cy_2_pos < 0) ? 0 : cy_2_pos;

}








void adjust_b(int16_t adjustment)
{
  if( (b_percent + adjustment) <= 100 &&  (b_percent + adjustment) >= 0)
    {
      b_percent = b_percent + adjustment; 
      int excess = (a_percent + b_percent + c_percent + d_percent) - 100;

      if(excess != 0 )
          {
            a_percent = a_percent - excess;
            excess = 0;
            if (a_percent < 0)
              {
                excess = 0 - a_percent;
                a_percent = 0;
              }
             else if (a_percent > 100)
              {
                excess = a_percent - 100;
                a_percent = 100;
              }        
          }
          
       if(excess != 0)
          {   
            c_percent = c_percent - excess;
            excess = 0;
            if (c_percent < 0)
               {
                 excess = 0 - c_percent;
                 c_percent = 0;
                }
            else if (c_percent > 100)
              {
                excess = c_percent - 100;
                c_percent = 100;
              }     
          }
          
        if(excess != 0)
           {
            d_percent = d_percent - excess;
            excess = 0;
           }
    }
}


void adjust_c(int16_t adjustment)
{
  if( (c_percent + adjustment) <= 100 &&  (c_percent + adjustment) >= 0)
    {
      c_percent = c_percent + adjustment; 
      int excess = (a_percent + b_percent + c_percent + d_percent) - 100;

      if(excess != 0 )
          {
            a_percent = a_percent - excess;
            excess = 0;
            if (a_percent < 0)
              {
                excess = 0 - a_percent;
                a_percent = 0;
              }
             else if (a_percent > 100)
              {
                excess = a_percent - 100;
                a_percent = 100;
              }        
          }
          
       if(excess != 0)
          {   
            b_percent = b_percent - excess;
            excess = 0;
            if (b_percent < 0)
               {
                 excess = 0 - b_percent;
                 b_percent = 0;
                }
            else if (b_percent > 100)
              {
                excess = b_percent - 100;
                b_percent = 100;
              }     
          }
          
        if(excess != 0)
           {
            d_percent = d_percent - excess;
            excess = 0;
           }
    }
}



void adjust_d(int16_t adjustment)
{
  if( (d_percent + adjustment) <= 100 &&  (d_percent + adjustment) >= 0)
    {
      d_percent = d_percent + adjustment; 
      int excess = (a_percent + b_percent + c_percent + d_percent) - 100;

      if(excess != 0 )
          {
            a_percent = a_percent - excess;
            excess = 0;
            if (a_percent < 0)
              {
                excess = 0 - a_percent;
                a_percent = 0;
              }
             else if (a_percent > 100)
              {
                excess = a_percent - 100;
                a_percent = 100;
              }        
          }
          
       if(excess != 0)
          {   
            b_percent = b_percent - excess;
            excess = 0;
            if (b_percent < 0)
               {
                 excess = 0 - b_percent;
                 b_percent = 0;
                }
            else if (b_percent > 100)
              {
                excess = b_percent - 100;
                b_percent = 100;
              }     
          }
          
        if(excess != 0)
           {
            c_percent = c_percent - excess;
            excess = 0;
           }
    }
}
