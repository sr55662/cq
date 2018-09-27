
static const int CQ_MSG_LEN = 4;
static const int CQ_MSG_HDR_LEN = 24;
static const int CQ_OLD_MSG_HDR_LEN = 16;
static const int CQ_MSG_CAT_LEN = 1;
static const int CQ_MSG_TYPE_LEN = 1;
static const int CQ_MSG_NET_LEN = 1;
static const int CQ_RETRANS_REQ_LEN = 2;
static const int CQ_MSG_SEQ_NUM_LEN = 9;
static const int CQ_OLD_MSG_SEQ_NUM_LEN = 7;

static const int CQS_SQ_INSTRUMENT_ID_LEN = 3;
static const int CQS_QUOTE_TYPE_LEN = 1;
static const int CQS_SQ_PRICE_LEN = 8;
static const int CQS_SQ_SIZE_LEN = 3;
static const int CQS_LQ_INSTRUMENT_ID_LEN = 11;
static const int CQS_LQ_PRICE_LEN = 12;
static const int CQS_LQ_SIZE_LEN = 7;

static const int CQS_LONG_NATIONAL_BBO_LEN = 58;
static const int CQS_SHORT_NATIONAL_BBO_LEN = 28;
static const int CQS_NASDAQ_BBO_LEN = 56;

static const int CTS_ST_INSTRUMENT_ID_LEN = 3;
static const int CTS_ST_TRADE_TYPE_LEN = 1;
static const int CTS_ST_SIZE_LEN = 4;
static const int CTS_ST_PRICE_LEN = 8;
static const int CTS_LT_INSTRUMENT_ID_LEN = 11;
static const int CTS_LT_TRADE_TYPE_LEN = 4;
static const int CTS_LT_PRICE_LEN = 12;
static const int CTS_LT_SIZE_LEN = 9;
static const int CTS_INSTRUMENT_STATUS_LEN = 1;
static const int CTS_HALT_REASON_LEN = 1;

int CQDecoder::getDenominator(char code)
{
  switch (code)
  {
    case 'A': return 10;
    case 'B': return 100;
    case 'C': return 1000;
    case 'D': return 10000;
    case 'E': return 100000;
    case 'F': return 1000000;
    case 'G': return 10000000;
    case 'H': return 100000000;
    case 'I': return 1;
    default: return 1;
  };
}

int CQDecoder::decodeHeader(const char* buf, int len, MsgHeader& hdr)
{
  if (len < CQ_MSG_HDR_LEN + 1) // SOH Character
    return 0;

  char* msgPtr = (char*)buf;

  msgPtr++; // SOH Character

  // Check for CTS Imbalance message 
  if (msgPtr[0] == 'E' && msgPtr[1] == 'F' ) 
  {
    char sstatus = msgPtr[CQ_MSG_HDR_LEN + 21];
    char hreason = msgPtr[CQ_MSG_HDR_LEN + 22];
    if ( sstatus == '7' || sstatus == '8' || sstatus == '9' || sstatus == 'A' || hreason == 'I' )
      msgPtr[0] = 'I';
  }

  hdr.msgType(msgPtr);
  msgPtr += CQ_MSG_CAT_LEN + CQ_MSG_TYPE_LEN + CQ_MSG_NET_LEN;

  char retransmissionRequester = msgPtr[0];
  // If this isn't an original message ('O'), do not process 
  if (retransmissionRequester  != 'O')
    return -1;

  msgPtr += CQ_RETRANS_REQ_LEN;

  msgPtr += 3;
    
  hdr.msgSeqNum(fast_atoi(msgPtr, CQ_MSG_SEQ_NUM_LEN));
  msgPtr += CQ_MSG_SEQ_NUM_LEN;

  // Participant ID, 1 char, NYSE ('N'), AMEX ('A')
  hdr.msgSource(msgPtr[0]);
  msgPtr++;

  int hrs = msgPtr[0] - '0';
  hrs *= 3600000;
  int min = msgPtr[1] - '0';
  min *= 60000;
  int sec = msgPtr[2] - '0';
  sec *= 1000;
  int msec = ((msgPtr[3] - '0') * 100) + ((msgPtr[4] - '0') * 10) + (msgPtr[5] - '0');
  hdr.exchTimestamp(hrs + min + sec + msec);
  hdr.msgHeaderLength(CQ_MSG_HDR_LEN);
  return CQ_MSG_HDR_LEN + 1;
}

int CQDecoder::decodeShortNBBO(const char* buf, QuoteMsg& bidMsg, QuoteMsg& askMsg)
{
  const char* buf0 = buf;
  buf++; // Participant Id
  char priceDenominator = buf[0];
  buf++;
  double price = fast_atoi(buf, CQS_SQ_PRICE_LEN);
  price /= getDenominator(priceDenominator);
  bidMsg.price(price);
  buf += CQS_SQ_PRICE_LEN;
  
  int size = fast_atoi(buf, CQS_SQ_SIZE_LEN);
  bidMsg.size(size);
  buf += CQS_SQ_SIZE_LEN; 
  buf++; // Reserved

  buf++; // Participant Id
  priceDenominator = buf[0];
  buf++;
  price = fast_atoi(buf, CQS_SQ_PRICE_LEN);
  price /= getDenominator(priceDenominator);
  askMsg.price(price);
  buf += CQS_SQ_PRICE_LEN;
  
  size = fast_atoi(buf, CQS_SQ_SIZE_LEN);
  askMsg.size(size);
  buf += CQS_SQ_SIZE_LEN; 
  buf++; // Reserved

  return buf - buf0;
}

int CQDecoder::decodeLongNBBO(const char* buf, QuoteMsg& bidMsg, QuoteMsg& askMsg)
{
  const char* buf0 = buf;
  buf += 2; // Reserved
  buf++; // Participant Id

  char priceDenominator = buf[0];
  buf++;
  double price = fast_atoi(buf, CQS_LQ_PRICE_LEN);
  price /= getDenominator(priceDenominator);
  bidMsg.price(price);
  buf += CQS_LQ_PRICE_LEN;
  
  int size = fast_atoi(buf, CQS_LQ_SIZE_LEN);
  bidMsg.size(size);
  buf += CQS_LQ_SIZE_LEN; 

  buf += 7;

  buf++; // Participant Id
  priceDenominator = buf[0];
  buf++;
  price = fast_atoi(buf, CQS_LQ_PRICE_LEN);
  price /= getDenominator(priceDenominator);
  askMsg.price(price);
  buf += CQS_LQ_PRICE_LEN;

  size = fast_atoi(buf, CQS_LQ_SIZE_LEN);
  askMsg.size(size);
  buf += CQS_LQ_SIZE_LEN; 

  buf += 7;

  return buf - buf0;
}

int CQDecoder::decodeQuoteMsg(const char* buf, MsgHeader& hdr, QuoteList& list)
{
  int i;
  const char* buf0 = buf;
  char symbol[INSTRUMENT_ID_LEN];
  QuoteMsg bidMsg;
  bidMsg.side('B');
  bidMsg.exchTimestamp(hdr.exchTimestamp());
  QuoteMsg askMsg;
  askMsg.side('A');
  askMsg.exchTimestamp(hdr.exchTimestamp());
  if (hdr.msgType()[1] == 'D') // Short Quote
  {
    for (i = 0; i < CQS_SQ_INSTRUMENT_ID_LEN; i++)
    {
      if (buf[i] == ' ')
        break;
      else
        symbol[i] = buf[i];
    }
    if (!nbboFlag_)
    {
      symbol[i++] = '.';
      symbol[i++] = hdr.msgSource();
    }
    else if (nbboSuffix_)
    {
      symbol[i++] = '.';
      symbol[i++] = nbboSuffix_;
    }
    bidMsg.instrumentId(symbol, i);
    buf += CQS_SQ_INSTRUMENT_ID_LEN;
    bidMsg.quoteType(buf, CQS_QUOTE_TYPE_LEN);
    askMsg.quoteType(buf, CQS_QUOTE_TYPE_LEN);
    buf += CQS_QUOTE_TYPE_LEN;
    buf += 2; // Reserved

    char priceDenominator = buf[0];
    buf++;
    
    double price = fast_atoi(buf, CQS_SQ_PRICE_LEN);
    price /= getDenominator(priceDenominator);
    bidMsg.price(price);
    buf += CQS_SQ_PRICE_LEN;
  
    int size = fast_atoi(buf, CQS_SQ_SIZE_LEN);
    bidMsg.size(size);
    buf += CQS_SQ_SIZE_LEN; 

    buf++; // Reserved

    priceDenominator = buf[0];
    buf++;

    price = fast_atoi(buf, CQS_SQ_PRICE_LEN);
    price /= getDenominator(priceDenominator);
    askMsg.price(price);
    buf += CQS_SQ_PRICE_LEN;

    size = fast_atoi(buf, CQS_SQ_SIZE_LEN);
    askMsg.size(size);
    buf += CQS_SQ_SIZE_LEN; 
    buf++; // Reserved
    char nationalBBOInd = buf[0];
    buf++;
    char nasdaqBBOInd = buf[0];
    buf++;
    if (nbboFlag_)
    {
      if (nationalBBOInd == '4')
        buf += decodeLongNBBO(buf, bidMsg, askMsg);
      else if (nationalBBOInd == '6')
        buf += decodeShortNBBO(buf, bidMsg, askMsg);
      else if (nationalBBOInd != '1')
      {
        if (nasdaqBBOInd == '3')
          buf += CQS_NASDAQ_BBO_LEN;
        hdr.msgBodyLength(buf - buf0 + 1);
        return 0;
      }
    }
    else
    {
      if (nationalBBOInd == '4')
        buf += CQS_LONG_NATIONAL_BBO_LEN;
      else if (nationalBBOInd == '6')
        buf += CQS_SHORT_NATIONAL_BBO_LEN;
    }
    if (nasdaqBBOInd == '3')
      buf += CQS_NASDAQ_BBO_LEN;

    list.list().push_back(bidMsg);
    list.list().push_back(askMsg);
  }
  else if (hdr.msgType()[1] == 'B') // Long Quote
  {
    for (i = 0; i < CQS_LQ_INSTRUMENT_ID_LEN; i++)
    {
      if (buf[i] == ' ')
        break;
      else
        symbol[i] = buf[i];
    }
    if (!nbboFlag_)
    {
      symbol[i++] = '.';
      symbol[i++] = hdr.msgSource();
    }
    else if (nbboSuffix_)
    {
      symbol[i++] = '.';
      symbol[i++] = nbboSuffix_;
    }
    bidMsg.instrumentId(symbol, i);
    buf += CQS_LQ_INSTRUMENT_ID_LEN;
    buf += 13;

    bidMsg.quoteType(buf, CQS_QUOTE_TYPE_LEN);
    askMsg.quoteType(buf, CQS_QUOTE_TYPE_LEN);
    buf += CQS_QUOTE_TYPE_LEN;
    buf += 2; // Reserved

    char priceDenominator = buf[0];
    buf++;
    
    double price = fast_atoi(buf, CQS_LQ_PRICE_LEN);
    price /= getDenominator(priceDenominator);
    bidMsg.price(price);
    buf += CQS_LQ_PRICE_LEN;
  
    int size = fast_atoi(buf, CQS_LQ_SIZE_LEN);
    bidMsg.size(size);
    buf += CQS_LQ_SIZE_LEN; 

    priceDenominator = buf[0];
    buf++;

    price = fast_atoi(buf, CQS_LQ_PRICE_LEN);
    price /= getDenominator(priceDenominator);
    askMsg.price(price);
    buf += CQS_LQ_PRICE_LEN;

    size = fast_atoi(buf, CQS_LQ_SIZE_LEN);
    askMsg.size(size);
    buf += CQS_LQ_SIZE_LEN; 

    buf += 9;

    char nationalBBOInd = buf[0];
    buf++;
    char nasdaqBBOInd = buf[0];
    buf++;

    if (nbboFlag_)
    {
      if (nationalBBOInd == '4')
        buf += decodeLongNBBO(buf, bidMsg, askMsg);
      else if (nationalBBOInd == '6')
        buf += decodeShortNBBO(buf, bidMsg, askMsg);
      else if (nationalBBOInd != '1')
      {
        if (nasdaqBBOInd == '3')
          buf += CQS_NASDAQ_BBO_LEN;
        hdr.msgBodyLength(buf - buf0 + 1);
        return 0;
      }
    }
    else
    {
      if (nationalBBOInd == '4')
        buf += CQS_LONG_NATIONAL_BBO_LEN;
      else if (nationalBBOInd == '6')
        buf += CQS_SHORT_NATIONAL_BBO_LEN;
    }
    if (nasdaqBBOInd == '3')
      buf += CQS_NASDAQ_BBO_LEN;

    list.list().push_back(bidMsg);
    list.list().push_back(askMsg);
  }
  hdr.msgBodyLength(buf - buf0 + 1);
  return 1;
}

int CQDecoder::decodeTradeMsg(const char* buf, MsgHeader& hdr, TradeMsg& msg)
{
  msg.exchTimestamp(hdr.exchTimestamp());
  int i;
  const char* buf0 = buf;
  char symbol[INSTRUMENT_ID_LEN];
  if (hdr.msgType()[1] == 'I') // Short Trade
  {
    for (i = 0; i < CTS_ST_INSTRUMENT_ID_LEN; i++)
    {
      if (buf[i] == ' ')
        break;
      else
        symbol[i] = buf[i];
    }
    if (!nbboFlag_)
    {
      symbol[i++] = '.';
      symbol[i++] = hdr.msgSource();
    }
    else if (nbboSuffix_)
    {
      symbol[i++] = '.';
      symbol[i++] = nbboSuffix_;
    }
    msg.instrumentId(symbol, i);
    buf += CTS_ST_INSTRUMENT_ID_LEN;
    msg.tradeType(buf, CTS_ST_TRADE_TYPE_LEN);
    if (buf[0] == '@' || buf[0] == 'E' || buf[0] == 'F')
      msg.isCacheable(true);
    buf += CTS_ST_TRADE_TYPE_LEN;

    int size = fast_atoi(buf, CTS_ST_SIZE_LEN);
    msg.size(size);
    buf += CTS_ST_SIZE_LEN; 

    char priceDenominator = buf[0];
    buf++;
    
    double price = fast_atoi(buf, CTS_ST_PRICE_LEN);
    price /= getDenominator(priceDenominator);
    msg.price(price);
    buf += CTS_ST_PRICE_LEN;

    buf += 3;
  }
  else if (hdr.msgType()[1] == 'B') // Long Trade
  {
    for (i = 0; i < CTS_LT_INSTRUMENT_ID_LEN; i++)
    {
      if (buf[i] == ' ')
        break;
      else
        symbol[i] = buf[i];
    }
    if (!nbboFlag_)
    {
      symbol[i++] = '.';
      symbol[i++] = hdr.msgSource();
    }
    else if (nbboSuffix_)
    {
      symbol[i++] = '.';
      symbol[i++] = nbboSuffix_;
    }
    msg.instrumentId(symbol, i);
    buf += CTS_LT_INSTRUMENT_ID_LEN;

    buf += 14;

    int j = 0;
    char tradeType[TRADE_TYPE_LEN];
    if (buf[0] != ' ')
    {
      tradeType[j++] = buf[0];
    }
    else
    {
      tradeType[j++] = buf[1];
      tradeType[j++] = buf[2];
      tradeType[j++] = buf[3];
    }
    msg.tradeType(tradeType, j);

    if ((tradeType[0] == '@' || tradeType[0] == 'F' || tradeType[2] == 'E') &&
        (tradeType[1] == ' '))
      msg.isCacheable(true);
    buf += CTS_LT_TRADE_TYPE_LEN;

    buf += 3;

    char priceDenominator = buf[0];
    buf++;
    
    double price = fast_atoi(buf, CTS_LT_PRICE_LEN);
    price /= getDenominator(priceDenominator);
    msg.price(price);
    buf += CTS_LT_PRICE_LEN;

    int size = fast_atoi(buf, CTS_LT_SIZE_LEN);
    msg.size(size);
    buf += CTS_LT_SIZE_LEN; 

    buf += 4;
  }
  hdr.msgBodyLength(buf - buf0 + 1);
  return 1;
}

int CQDecoder::decodeInstrumentStatusMsg(const char* buf, MsgHeader& hdr, InstrumentStatusMsg& msg)
{
  msg.exchTimestamp(hdr.exchTimestamp());
  int i;
  const char* buf0 = buf;
  char symbol[INSTRUMENT_ID_LEN];
  for (i = 0; i < CTS_LT_INSTRUMENT_ID_LEN; i++)
  {
    if (buf[i] == ' ')
      break;
    else
      symbol[i] = buf[i];
  }
  if (!nbboFlag_)
  {
    symbol[i++] = '.';
    symbol[i++] = hdr.msgSource();
  }
  else if (nbboSuffix_)
  {
    symbol[i++] = '.';
    symbol[i++] = nbboSuffix_;
  }
  msg.instrumentId(symbol, i);
  buf += CTS_LT_INSTRUMENT_ID_LEN;
  buf += 10;
  msg.instrumentStatusType(buf, CTS_INSTRUMENT_STATUS_LEN + CTS_HALT_REASON_LEN);
  buf += CTS_INSTRUMENT_STATUS_LEN + CTS_HALT_REASON_LEN;
  buf += 67;
  hdr.msgBodyLength(buf - buf0 + 1);
  return 1;
}

int CQDecoder::decodeTradeImbalanceMsg(const char* buf, MsgHeader& hdr, ImbalanceMsg& msg)
{
  int i;
  char symbol[INSTRUMENT_ID_LEN];

  msg.exchTimestamp(hdr.exchTimestamp());
  for (i = 0; i < CTS_LT_INSTRUMENT_ID_LEN; i++)
  {
    if (buf[i] == ' ')
      break;
    else
      symbol[i] = buf[i];
  }
  if (!nbboFlag_)
  {
    symbol[i++] = '.';
    symbol[i++] = hdr.msgSource();
  }
  else if (nbboSuffix_)
  {
    symbol[i++] = '.';
    symbol[i++] = nbboSuffix_;
  }
  symbol[i++] = 'I';

  msg.instrumentStr.assign(symbol, i); // Symbol or Instrument ID
  buf += CTS_LT_INSTRUMENT_ID_LEN + 10;
  char secstatus = buf[0]; //Security Status 
  msg.typeVec.push_back(secstatus); buf++;
  //char haltreason = buf[0]; //Halt Reason 
  //msg.typeVec.push_back(haltreason); 
  buf += 3;
  char priceDenominator  = buf[0]; // Last Price Denominator Index
  buf++;
  float price = fast_atoi(buf, CTS_LT_PRICE_LEN);
  price /= getDenominator(priceDenominator);
  msg.priceVec.push_back(price); // Last Price
  buf += CTS_LT_PRICE_LEN ;
  buf++;
  priceDenominator  = buf[0]; // High Indication Price Denominator Index
  buf++;
  price = fast_atoi(buf, CTS_LT_PRICE_LEN);
  price /= getDenominator(priceDenominator);
  msg.priceVec.push_back(price); // High Indication Price
  buf += CTS_LT_PRICE_LEN;
  priceDenominator  = buf[0]; // Low Price Denominator Index
  buf++;
  price = fast_atoi(buf, CTS_LT_PRICE_LEN);
  price /= getDenominator(priceDenominator);
  msg.priceVec.push_back(price); // Low Indication Price
  buf += CTS_LT_PRICE_LEN +1;
  int size = fast_atoi(buf, CTS_LT_SIZE_LEN);
  msg.sizeVec.push_back(size); // Buy Volume
  buf += CTS_LT_SIZE_LEN; 
  size = fast_atoi(buf, CTS_LT_SIZE_LEN);
  msg.sizeVec.push_back(size); // Sell Volume
  buf += CTS_LT_SIZE_LEN + 6; 
 
  return 1;
}

int CQDecoder::decodeImbalanceMsg(const char* buf, MsgHeader& hdr, ImbalanceMsg& msg)
{
   const std::string mtype = hdr.msgType();

   if (mtype[1] == 'F')
      return decodeTradeImbalanceMsg(buf, hdr, msg);
    
   return -1;
}

X_Message_Block* CQDecoder::outTradeMsg(const ImbalanceMsg &msg ) const
{
  char instrumentId[INSTRUMENT_ID_LEN+1];
  memcpy(instrumentId, msg.instrumentStr.c_str(), msg.instrumentStr.length()+1);
  X_Date_Time recvDate(msg.recvTime());
  size_t msgSize = X_static_cast(size_t, XY_HDR_LEN + XY_CQ_TRADE_IMB_MSG_LEN);

  X_Message_Block *mb;
  mb = new X_Message_Block(msgSize);
#ifdef __XY_BINARY_HEADER__
  unsigned char vTmp = XY_HDR_MAJOR_VERSION;
  mb->copy((char*)&vTmp, XY_HDR_MAJOR_VER_LEN);
  vTmp = XY_HDR_MINOR_VERSION;
  mb->copy((char*)&vTmp, XY_HDR_MINOR_VER_LEN);
  X_UINT32 sizeTmp = htoll(msgSize);
  mb->copy((char*)&sizeTmp, XY_HDR_MSG_LEN);
  sizeTmp = htoll(1);
  mb->copy((char*)&sizeTmp, XY_HDR_NUM_MSG_LEN);
  sizeTmp = htoll(((((recvDate.hour() * 60) + recvDate.minute()) * 60) + recvDate.second()) * 1000 + recvDate.microsec()/1000);
  mb->copy((char*)&sizeTmp, XY_HDR_TS_LEN);
  mb->copy((char*)"N", 1);
#else
  char header[XY_HDR_LEN+1];
  sprintf(header, "%04d%02ld%02ld%02ld%03ld001I",
      msgSize,
      recvDate.hour(),
      recvDate.minute(),
      recvDate.second(),
      recvDate.microsec()/1000);
  mb->copy(header, XY_HDR_LEN);
#endif
  mb->copy(instrumentId, INSTRUMENT_ID_LEN); // Symbol or Instrument ID
  mb->copy((char*)&msg.typeVec[0], sizeof(char)); // Security Status
  X_UINT32 uintTmp = htoll(*(X_UINT32*)(char*)&msg.priceVec[0]);
  mb->copy((const char*)&uintTmp, sizeof(uintTmp)); // Last Price
  uintTmp = htoll(*(X_UINT32*)(char*)&msg.priceVec[1]);
  mb->copy((const char*)&uintTmp, sizeof(uintTmp)); // High Indication Price
  uintTmp = htoll(*(X_UINT32*)(char*)&msg.priceVec[2]);
  mb->copy((const char*)&uintTmp, sizeof(uintTmp)); // Low Indication Price 
  uintTmp = htoll(msg.sizeVec[0]); // Buy Volume
  mb->copy((char*)&uintTmp, sizeof(uintTmp));
  uintTmp = htoll(msg.sizeVec[1]); // Sell Volume
  mb->copy((char*)&uintTmp, sizeof(uintTmp));

  return mb;
}

X_Message_Block*  CQDecoder::outMsg(const ImbalanceMsg& msg, const MsgHeader& hdr) const
{
   const std::string msgtype = hdr.msgType();

   if (msgtype[1] == 'F')
      return outTradeMsg(msg);

   return NULL;
}

int CQDecoder::decodeStatusMsg(const char* buf, MsgHeader& hdr, StatusMsg& msg)
{
  msg.exchTimestamp(hdr.exchTimestamp());
  msg.statusType('M');
  int i = 0;
  // Check for Unit Separator (US) and End of Text (ETX)
  while (buf[i] != 0x1F && buf[i] != 0x03)
    i++;
  hdr.msgBodyLength(i + 1);
  // truncation, 300 character potential
  if (i > STATUS_TEXT_LEN-1)
    i = STATUS_TEXT_LEN-1;
  msg.statusText(buf, i);
  return 1;
}

bool CQDecoder::updateMessageTimestamp(char *packet,const X_Date_Time & recvTime)
{
  char *buf = packet;

  if ( !buf )
    return false;
  
  buf += CQ_MSG_CAT_LEN + CQ_MSG_TYPE_LEN + CQ_MSG_NET_LEN + CQ_RETRANS_REQ_LEN +
            CQ_MSG_SEQ_NUM_LEN + 5;
  char tmpchar;
  long millisecs = recvTime.microsec()/1000;
  tmpchar = recvTime.hour() + '0'; buf[0] = tmpchar;buf++;
  tmpchar = recvTime.minute() + '0'; buf[0] = tmpchar;buf++;
  tmpchar = recvTime.second() + '0'; buf[0] = tmpchar;buf++;
  tmpchar = millisecs/100 + '0'; buf[0] = tmpchar;buf++;
  tmpchar = millisecs/10 + '0'; buf[0] = tmpchar;buf++;
  tmpchar = millisecs%10 + '0'; buf[0] = tmpchar;

  return true;
}

int CQDecoder::msgLength(const MsgHeader& hdr, const char* buf, const int len)
{
  if (hdr.msgBodyLength() != 0)
  {
    int length = hdr.msgHeaderLength() + hdr.msgBodyLength();
    // If this is the last message, increment by one for the ETX character
    if (length + 1 == len)
      return length + 1;
    else
      return length;
  }
  else
  {
    int msgLen = hdr.msgHeaderLength();
    while (msgLen < len && buf[msgLen] != 0x1F)
      msgLen++;
    return msgLen;
  }
}

Decoder::MsgType CQDecoder::msgType(const char* type)
{
  if (type[0] == 'E')
  {
    if (type [1] == 'D')
    {
      if (type[2] == 'E' || type[2] == 'F')
        return Decoder::QUOTE_MSG_TYPE;
    }
    else if (type[1] == 'B')
    {
      if (type[2] == 'E' || type[2] == 'F')
        return Decoder::QUOTE_MSG_TYPE;
      else if (type[2] == 'A' || type[2] == 'B' || type[2] == 'C')
        return Decoder::TRADE_MSG_TYPE;
    }
    else if (type[1] == 'I')
    {
      if (type[2] == 'A' || type[2] == 'B' || type[2] == 'C')
        return Decoder::TRADE_MSG_TYPE;
    }
    else if (type[1] == 'F')
    {
      return INSTRUMENT_STATUS_MSG_TYPE;
    }
    return Decoder::NULL_MSG_TYPE;
  }
  else if (type[0] == 'C' && type[1] == 'L')
  {
    return Decoder::SEQUENCE_RESET_MSG_TYPE;
  }
  else if (type[0] == 'A' && type[1] == 'H')
  {
    return Decoder::STATUS_MSG_TYPE;
  }
  else if (type[0] == 'I' )
  {
    return Decoder::IMBALANCE_MSG_TYPE;
  }
  else
  {
    return Decoder::NULL_MSG_TYPE;
  } 
}
