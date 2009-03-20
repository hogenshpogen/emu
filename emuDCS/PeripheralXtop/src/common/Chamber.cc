//-----------------------------------------------------------------------
// $Id: Chamber.cc,v 1.5 2009/03/20 13:08:14 liu Exp $
// $Log: Chamber.cc,v $
// Revision 1.5  2009/03/20 13:08:14  liu
// move include files to include/emu/pc
//
// Revision 1.4  2008/11/03 20:00:13  liu
// do not update timestamp for all-zero chamber
//
// Revision 1.3  2008/10/18 18:18:44  liu
// update
//
// Revision 1.2  2008/10/13 12:14:28  liu
// update
//
// Revision 1.1  2008/10/12 11:55:49  liu
// new release of e2p code
//

#include "emu/pc/Chamber.h"

namespace emu {
  namespace e2p {

//
Chamber::Chamber():
label_("CSC"), active_(0), ready_(false), corruption(false)
{
}

Chamber::~Chamber()
{
}

void Chamber::Fill(char *buffer, int source)
{
   int idx=0, i;
   char *start = buffer, *item, *sep = " ";
   char *last=NULL;
   float y;

   item=strtok_r(start, sep, &last);
   while(item)
   {
       if(idx<3) 
       {
           i = atoi(item);
           if(source) 
             {  states_bk[idx] = i; 
             }
           states[idx] = i; 
       }
       else if(idx<52)
       {  
           y=strtof(item,NULL);
           if(source) values_bk[idx-3]=y;
           values[idx-3]=y;
       }
       idx++;
       item=strtok_r(NULL, sep, &last);
   };
   if(source && idx==51) 
   {   
       old_time_lv = states_bk[1];
       old_time_temp = states_bk[1];
       ready_ = true;
   }
   if(idx!=51 || values[47]!=(-50.))
   {   std::cout << "BAD...total " << idx << " last one " << values[47] << std::endl;
       corruption = true;
   }
   else corruption = false;
}

//   hint (operation mode)
//    0        default, selected by EmuDim
//    1        file value, all good
//    2        reading error removed
//    3        true reading with all errors

void Chamber::GetDimLV(int hint, LV_1_DimBroker *dim_lv )
{
   int *info, this_st, total_st=0;
   float *data;
   char *vcc_ip = "02:00:00:00:00:00";
   //   float V33, V50, V60, C33, C50, C60, V18, V55, V56, C18, C55, C56;

   if(corruption || hint==1)
   {  info = &(states_bk[0]);
      data = &(values_bk[0]);
   }
   else
   {  info = &(states[0]);
      data = &(values[0]);
      if(hint==2)
      {
        float allc33= data[0]+data[3]+data[6]+data[9];
        if( allc33 >10.) data = &(values_bk[0]);
      }
   }      
   for(int i=0; i<CFEB_NUMBER; i++)
   {
      dim_lv->cfeb.v33[i] = data[19+3*i];
      dim_lv->cfeb.v50[i] = data[20+3*i];
      dim_lv->cfeb.v60[i] = data[21+3*i];
      dim_lv->cfeb.c33[i] = data[ 0+3*i];
      dim_lv->cfeb.c50[i] = data[ 1+3*i];
      dim_lv->cfeb.c60[i] = data[ 2+3*i];
      this_st=info[0];
      if(data[19+3*i]==0. && data[20+3*i]==0. && data[21+3*i]==0.) this_st=0;
      dim_lv->cfeb.status[i] = this_st;
      total_st += this_st;
   }
      dim_lv->alct.v18 = data[35];
      dim_lv->alct.v33 = data[34];
      dim_lv->alct.v55 = data[36];
      dim_lv->alct.v56 = data[37];
      dim_lv->alct.c18 = data[16];
      dim_lv->alct.c33 = data[15];
      dim_lv->alct.c55 = data[17];
      dim_lv->alct.c56 = data[18];
      this_st=info[0];
      if(data[34]==0. && data[35]==0. && data[36]==0. && data[37]==0. ) this_st=0;
      dim_lv->alct.status = this_st;
      total_st += this_st;
   
   if(total_st)
   {  dim_lv->update_time = info[1];
      old_time_lv = info[1];
   }
   else
      dim_lv->update_time = old_time_lv;
   dim_lv->slot = (states_bk[2]>>8)&0xFF;

   memcpy(dim_lv->VCCMAC, vcc_ip, 18);
   sprintf(dim_lv->VCCMAC+15, "%02X", (states_bk[2]&0xFF));
}

void Chamber::GetDimTEMP(int hint, TEMP_1_DimBroker *dim_temp )
{
   int *info;
   float *data, total_temp;
   char *vcc_ip = "02:00:00:00:00:00";

   if(corruption || hint==1)
   {  info = &(states_bk[0]);
      data = &(values_bk[0]);
   }      
   else
   {  info = &(states[0]);
      data = &(values[0]);
      if(hint==2)
      {
         if(data[40]<-30 || data[43]< -30 )  data= &(values_bk[0]);
      }
   }
      dim_temp->t_daq = (data[40]<(-30)) ? 0.0 :data[40];
      dim_temp->t_cfeb1 = (data[41]<(-30)) ? 0.0 :data[41];
      dim_temp->t_cfeb2 = (data[42]<(-30)) ? 0.0 :data[42];
      dim_temp->t_cfeb3 = (data[43]<(-30)) ? 0.0 :data[43];
      dim_temp->t_cfeb4 = (data[44]<(-30)) ? 0.0 :data[44];
      // 1/3 chambers have no CFEB5
      dim_temp->t_cfeb5 = (data[45]<(-30)) ? 0.0 : data[45];
      dim_temp->t_alct = (data[46]<(-30)) ? 0.0 : data[46];
   
   total_temp = dim_temp->t_daq + dim_temp->t_cfeb1 + dim_temp->t_cfeb2
            + dim_temp->t_cfeb3 + dim_temp->t_cfeb4 + dim_temp->t_alct;
   if(total_temp>1.0)
   {
      dim_temp->update_time = info[1];
      old_time_temp = info[1];
   }
   else
      dim_temp->update_time = old_time_temp;
   dim_temp->slot = (states_bk[2]>>8)&0xFF;

   memcpy(dim_temp->VCCMAC, vcc_ip, 18);
   sprintf(dim_temp->VCCMAC+15, "%02X", (states_bk[2]&0xFF));
}

  } // namespace emu::e2p
} // namespace emu
