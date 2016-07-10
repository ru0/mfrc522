/******************************************************************************
 * 包含文件
 ******************************************************************************/
#include <bcm2835.h>
#include "RFID.h"
 



void writeMFRC522(unsigned char Address, unsigned char value);
void antennaOn(void);
 
/******************************************************************************
 * 用户 API
 ******************************************************************************/
 
/******************************************************************************
 * 函 数 名：init
 * 功能描述：初始化RC522
 * 输入参数：无
 * 返 回 值：无
 ******************************************************************************/
void init()
{
 
  //SPI
  bcm2835_spi_begin();
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); 
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_65536); 
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0); 
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
 
  writeMFRC522(CommandReg, PCD_RESETPHASE); // 写空闲命令
 
  writeMFRC522(TModeReg, 0x8D);      // Tauto=1; f(Timer) = 6.78MHz/TPreScaler 定义内部定时器设置
  writeMFRC522(TPrescalerReg, 0x3E); // TModeReg[3..0] + TPrescalerReg

  writeMFRC522(TReloadRegL, 30);     // 16位定时器低位
  writeMFRC522(TReloadRegH, 0);      // 16位定时器高位
  writeMFRC522(TxAutoReg, 0x40);     // 调制发送信号为100%ASK
  writeMFRC522(ModeReg, 0x3D);       // CRC valor inicial de 0x6363
 
  antennaOn();    //打开天线
}
 
/******************************************************************************
 * 函 数 名：writeMFRC522
 * 功能描述：向MFRC522的某一寄存器写一个字节数据
 * 输入参数：addr--寄存器地址；val--要写入的值
 * 返 回 值：无
 ******************************************************************************/
void writeMFRC522(unsigned char Address, unsigned char value)
{
    char buff[2];
 
    buff[0] = (char)((Address<<1)&0x7E);
    buff[1] = (char)value;
     
    bcm2835_spi_transfern(buff,2);  // Transfers any number of bytes to and from the currently selected SPI slave using bcm2835_spi_transfernb. 从当前选定的SPI传输一字节到从设备 The returned data from the slave replaces the transmitted data in the buffer. 
}
 
/******************************************************************************
 * 函 数 名：readMFRC522
 * 功能描述：从MFRC522的某一寄存器读一个字节数据
 * 输入参数：addr--寄存器地址
 * 返 回 值：返回读取到的一个字节数据
 ******************************************************************************/
unsigned char readMFRC522(unsigned char Address)
{
    char buff[2];
    buff[0] = ((Address<<1)&0x7E)|0x80; //0x40  1000 0000
	/* 变化成有效的地址形式
	   最低位为0，最高位为1时候时，从MFRC522读数据
	   最低位为0，最高位为0时候时，往MFRC522写数据
	 */
    bcm2835_spi_transfern(buff,2);
    return (uint8_t)buff[1];
}
 
/******************************************************************************
 * 函 数 名：setBitMask
 * 功能描述：置RC522寄存器位
 * 输入参数：reg--寄存器地址;mask--置位值
 * 返 回 值：无
 ******************************************************************************/
void setBitMask(unsigned char reg, unsigned char mask)
{
  unsigned char tmp;
  tmp = readMFRC522(reg);
  writeMFRC522(reg, tmp | mask);  // set bit mask
}
 
/******************************************************************************
 * 函 数 名：clearBitMask
 * 功能描述：清RC522寄存器位
 * 输入参数：reg--寄存器地址;mask--清位值
 * 返 回 值：无
 ******************************************************************************/
void clearBitMask(unsigned char reg, unsigned char mask)
{
  unsigned char tmp;
  tmp = readMFRC522(reg);
  writeMFRC522(reg, tmp & (~mask));  // clear bit mask
}
 
/******************************************************************************
 * 函 数 名：antennaOn
 * 功能描述：开启天线,每次启动或关闭天险发射之间应至少有1ms的间隔
 * 输入参数：无
 * 返 回 值：无
 ******************************************************************************/
void antennaOn(void)
{
  unsigned char temp;
 
  temp = readMFRC522(TxControlReg);
  if (!(temp & 0x03))
  {
    setBitMask(TxControlReg, 0x03);
  }
}
 
/******************************************************************************
 * 函 数 名：antennaOff
 * 功能描述：关闭天线,每次启动或关闭天险发射之间应至少有1ms的间隔
 * 输入参数：无
 * 返 回 值：无
 ******************************************************************************/
void antennaOff(void)
{
  unsigned char temp;
 
  temp = readMFRC522(TxControlReg);
  if (!(temp & 0x03))
  {
    clearBitMask(TxControlReg, 0x03);
  }
}
 
/******************************************************************************
 * 函 数 名：calculateCRC
 * 功能描述：用MF522计算CRC
 * 输入参数：pIndata--要读数CRC的数据，len--数据长度，pOutData--计算的CRC结果
 * 返 回 值：无
 ******************************************************************************/
void calculateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
  unsigned char i, n;
 
  clearBitMask(DivIrqReg, 0x04);      //CRCIrq = 0
  setBitMask(FIFOLevelReg, 0x80);     //清FIFO指针
  //Write_MFRC522(CommandReg, PCD_IDLE);
 
  //向FIFO中写入数据
  for (i=0; i<len; i++)
    writeMFRC522(FIFODataReg, *(pIndata+i));
  writeMFRC522(CommandReg, PCD_CALCCRC);
 
  //等待CRC计算完成
  i = 0xFF;
  do
  {
    n = readMFRC522(DivIrqReg);
    i--;
  }
  while ((i!=0) && !(n&0x04));      //CRCIrq = 1
 
  //读取CRC计算结果
  pOutData[0] = readMFRC522(CRCResultRegL);
  pOutData[1] = readMFRC522(CRCResultRegM);
}
 
/******************************************************************************
 * 函 数 名：MFRC522ToCard
 * 功能描述：RC522和ISO14443卡通讯
 * 输入参数：command--MF522命令字，
 *           sendData--通过RC522发送到卡片的数据,
 *           sendLen--发送的数据长度
 *
 *           backData--接收到的卡片返回数据，
 *           backLen--返回数据的位长度
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char MFRC522ToCard(unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen)
{
  unsigned char status = MI_ERR;
  unsigned char irqEn = 0x00;
  unsigned char waitIRq = 0x00;
  unsigned char lastBits;
  unsigned char n;
  unsigned int i;
 
  switch (command)
  {
    case PCD_AUTHENT:   //认证卡密
    {
      irqEn = 0x12;
      waitIRq = 0x10;
      break;
    }
    case PCD_TRANSCEIVE:  //发送FIFO中数据
    {
      irqEn = 0x77;
      waitIRq = 0x30;
      break;
    }
    default:
      break;
  }
 
  writeMFRC522(CommIEnReg, irqEn|0x80); //允许中断请求
  clearBitMask(CommIrqReg, 0x80);       //清除所有中断请求位(复位)
  setBitMask(FIFOLevelReg, 0x80);       //FlushBuffer=1, FIFO初始化
 
  writeMFRC522(CommandReg, PCD_IDLE);   //无动作，取消当前命令
 
  //向FIFO中写入数据
  for (i=0; i < sendLen; i++)
    writeMFRC522(FIFODataReg, sendData[i]);
 
  //执行命令
  writeMFRC522(CommandReg, command);
  if (command == PCD_TRANSCEIVE)
    setBitMask(BitFramingReg, 0x80);    //StartSend=1,transmission of data starts
 
  //等待接收数据完成
  i = 2000; //i根据时钟频率调整，操作M1卡最大等待时间25ms
  do{
    //CommIrqReg[7..0]
    //Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
    n = readMFRC522(CommIrqReg);
    i--;
  }while ((i!=0) && !(n&0x01) && !(n&waitIRq));
 
  clearBitMask(BitFramingReg, 0x80);      //StartSend=0
 
  if (i != 0)
  {
    if(!(readMFRC522(ErrorReg) & 0x1B)) //BufferOvfl Collerr CRCErr ProtecolErr
    {
      status = MI_OK;
      if (n & irqEn & 0x01)
        status = MI_NOTAGERR;     //??
 
      if (command == PCD_TRANSCEIVE)
      {
        n = readMFRC522(FIFOLevelReg);
        lastBits = readMFRC522(ControlReg) & 0x07;
        if (lastBits)
          *backLen = (n-1)*8 + lastBits;
        else
          *backLen = n*8;
 
        if (n == 0)
          n = 1;
        if (n > MAX_LEN)
          n = MAX_LEN;
 
        //读取FIFO中接收到的数据
        for (i=0; i < n; i++)
          backData[i] = readMFRC522(FIFODataReg);
      }
    }
    else
      status = MI_ERR;
  }
 
  //SetBitMask(ControlReg,0x80);           //timer stops
  //Write_MFRC522(CommandReg, PCD_IDLE);
 
  return status;
}
 
 
/******************************************************************************
 * 函 数 名：findCard
 * 功能描述：寻卡，读取卡类型号
 * 输入参数：reqMode--寻卡方式，
 *                    0x52 = 寻感应区内所有符合14443A标准的卡
 *                    0x26 = 寻未进入休眠状态的卡
 *           TagType--返回卡片类型
 *                    0x4400 = Mifare_UltraLight
 *                    0x0400 = Mifare_One(S50)
 *                    0x0200 = Mifare_One(S70)
 *                    0x0800 = Mifare_Pro(X)
 *                    0x4403 = Mifare_DESFire
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char findCard(unsigned char reqMode, unsigned char *TagType)
{
  unsigned char status;
  unsigned int backBits;      //接收到的数据位数
 
  writeMFRC522(BitFramingReg, 0x07);    //TxLastBists = BitFramingReg[2..0] ??? (111)
 
  TagType[0] = reqMode; // default 0x52
  status = MFRC522ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);
 
  if ((status != MI_OK) || (backBits != 0x10))
    status = MI_ERR;
 
  return status;
}
 
/******************************************************************************
 * 函 数 名：anticoll
 * 功能描述：防冲突检测，读取选中卡片的卡序列号
 * 输入参数：serNum--返回4字节卡序列号,第5字节为校验字节
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char anticoll(unsigned char *serNum)
{
  unsigned char status;
  unsigned char i;
  unsigned char serNumCheck=0;
  unsigned int unLen;
 
  clearBitMask(Status2Reg, 0x08);   //TempSensclear 清MFCryptol On位
  clearBitMask(CollReg,0x80);     //ValuesAfterColl 清ValuesAfterColl 所有接收的位在冲突后被清除
  writeMFRC522(BitFramingReg, 0x00);    //TxLastBists = BitFramingReg[2..0] 清理寄存器 停止收发
 
  serNum[0] = PICC_ANTICOLL;
  serNum[1] = 0x20;
 
  status = MFRC522ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);
 
  if (status == MI_OK)
  {
    //校验卡序列号
    for (i=0; i<4; i++){
      *(serNum+i)  = serNum[i];
      serNumCheck ^= serNum[i];
    }
    if (serNumCheck != serNum[i]){
      status = MI_ERR;
    }
  }
 
  setBitMask(CollReg, 0x80);    //ValuesAfterColl=1
 
  return status;
}
 
/******************************************************************************
 * 函 数 名：auth
 * 功能描述：验证卡片密码
 * 输入参数：authMode--密码验证模式
 *                     0x60 = 验证A密钥
 *                     0x61 = 验证B密钥
 *           BlockAddr--块地址0～63
 *           Sectorkey--扇区密码
 *           serNum--卡片序列号，4字节
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char auth(unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum)
{
  unsigned char status;
  unsigned int recvBits;
  unsigned char i;
  unsigned char buff[12];
 
  //验证指令+块地址＋扇区密码＋卡序列号
  buff[0] = authMode;
  buff[1] = BlockAddr;
  for (i=0; i<6; i++)
    buff[i+2] = *(Sectorkey+i);
  for (i=0; i<4; i++)
    buff[i+8] = *(serNum+i);
     
  status = MFRC522ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);
  if ((status != MI_OK) || (!(readMFRC522(Status2Reg) & 0x08)))
    status = MI_ERR;
 
  return status;
}
 
/******************************************************************************
 * 函 数 名：read
 * 功能描述：读块数据
 * 输入参数：blockAddr--块地址;recvData--读出的块数据
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char read(unsigned char blockAddr, unsigned char *recvData)
{
  unsigned char status;
  unsigned int unLen;
 
  recvData[0] = PICC_READ;
  recvData[1] = blockAddr;
  calculateCRC(recvData,2, &recvData[2]);
  status = MFRC522ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);
 
  if ((status != MI_OK) || (unLen != 0x90))
    status = MI_ERR;
 
  return status;
}
 
/******************************************************************************
 * 函 数 名：write
 * 功能描述：写块数据
 * 输入参数：blockAddr--块地址;writeData--向块写16字节数据
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char write(unsigned char blockAddr, unsigned char *writeData)
{
  unsigned char status;
  unsigned int recvBits;
  unsigned char i;
  unsigned char buff[18];
 
  buff[0] = PICC_WRITE;
  buff[1] = blockAddr;
  calculateCRC(buff, 2, &buff[2]);
  status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);
 
  if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    status = MI_ERR;
 
  if (status == MI_OK)
  {
    for (i=0; i<16; i++)    //?FIFO?16Byte?? Datos a la FIFO 16Byte escribir
      buff[i] = *(writeData+i);
       
    calculateCRC(buff, 16, &buff[16]);
    status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);
 
    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
      status = MI_ERR;
  }
 
  return status;
}
 
/******************************************************************************
 * 函 数 名：selectTag
 * 功能描述：选卡，读取卡存储器容量
 * 输入参数：serNum--传入卡序列号,4字节卡序列号，第5字节为校验字节  
 * 返 回 值：成功返回卡容量
 ******************************************************************************/
unsigned char selectTag(unsigned char *serNum)
{
  unsigned char i;
  unsigned char status;
  unsigned char size;
  unsigned int recvBits;
  unsigned char buffer[9];
 
  //ClearBitMask(Status2Reg, 0x08);                        //MFCrypto1On=0
 
  buffer[0] = PICC_SElECTTAG;
  buffer[1] = 0x70;
 
  for (i=0; i<5; i++)
    buffer[i+2] = *(serNum+i);
 
  calculateCRC(buffer, 7, &buffer[7]);
   
  status = MFRC522ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
  if ((status == MI_OK) && (recvBits == 0x18))
    size = buffer[i];
  else
    size = 0;
  return size;
}
 
/******************************************************************************
 * 函 数 名：Halt
 * 功能描述：命令卡片进入休眠状态
 * 输入参数：无
 * 返 回 值：无
 ******************************************************************************/
void halt()
{
  unsigned char status;
  unsigned int unLen;
  unsigned char buff[4];
 
  buff[0] = PICC_HALT;
  buff[1] = 0;
  calculateCRC(buff, 2, &buff[2]);
 
  status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff,&unLen);
}