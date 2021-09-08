#include <iostream>
#include <iterator>
#include <string>
#include <unordered_set>
using namespace std;

/*AAC ADTS Header*/
/*  ISO/IEC 14496-3:2001(E)
 adts_frame()
 {
  byte_alignment()
  adts_fixed_header()
  adts_variable_header()
  adts_error_check()
  for( i=0; i<no_raw_data_blocks_in_frame+1; i++) {
    raw_data_block()
  }
 }
 7个字节
 adts_fixed_header()
 {
  Syncword                  12
  ID                        1
  Layer                     2
  protection_absent         1   //16
  Profile_ObjectType        2
  sampling_frequency_index  4
  private_bit               1   //25
  channel_configuration     3   //28
  original/copy             1
  home                      1
  Emphasis                  2   //32
 }
 |0   1   2   3   4   5   6   7   |
 |<--Synccword-->                 |
 |-sync-      |ID | layer | pa    |
 |pro_objT| samfrequency  |p  |ch |
 |chnnnel | o | h |Emphasi|cib|cis|
 |       aac_frame_length         |
 |  aac_frame_length  |adts_buffer|
 |            _fullness           |
 | no_raw |
 
 adts_variable_header()
 {
  copyright_identification_bit      1 bslbf
  copyright_identification_start    1 bslbf
  aac_frame_length                  13 bslbf
  adts_buffer_fullness              11 bslbf
  no_raw_data_blocks_in_frame       2 uimsbf
 }
 
 bsac_raw_data_block()
 {
  bsac_base_element();
  layer=slayer_size;
  while(data_available() && layer<(top_layer+slayer_size)) {
    bsac_layer_element(nch, layer);
    layer++;
  }
  byte_alignment();
 }
 
 bsac_base_element()
 {
    frame_length; 11 uimbf
    bsac_header();
    general_header();
    byte_alignment();
    for (slayer = 0; slayer < slayer_size; slayer++)
      bsac_layer_element(slayer);
 }
 
 samplingFrequencyIndex Value
 0x0                    96000
 0x1                    88200
 0x2                    64000
 0x3                    48000
 0x4                    44100
 0x5                    32000
 0x6                    24000
 0x7                    22050
 0x8                    16000
 0x9                    12000
 0xa                    11025
 0xb                    8000
 0xc                    7350
 0xd                    reserved
 0xe                    reserved
 0xf                    escape value
 */

//objectType(0_null 1_main 2_aaclc 3_aacssr 4_aacltp)
int FormatATDSHeader(int nObjType, int nFrameLength, int nChannels, int nSampleRate, uint8_t* sADTSHeader){
  if (sADTSHeader == NULL)
  {
    return;
  }
  int nRateIdx = 0;
  if (nSampleRate == 96000)
  {
    nRateIdx = 0;
  }
  else if (nSampleRate == 88200)
  {
    nRateIdx = 1;
  }
  else if (nSampleRate == 64000)
  {
    nRateIdx = 2;
  }
  else if (nSampleRate == 48000)
  {
    nRateIdx = 3;
  }
  else if (nSampleRate == 44100)
  {
    nRateIdx = 4;
  }
  else if (nSampleRate == 32000)
  {
    nRateIdx = 5;
  }
  else if (nSampleRate == 24000)
  {
    nRateIdx = 6;
  }
  else if (nSampleRate == 22050)
  {
    nRateIdx = 7;
  }
  else if (nSampleRate == 16000)
  {
    nRateIdx = 8;
  }
  else if (nSampleRate == 12000)
  {
    nRateIdx = 9;
  }
  else if (nSampleRate == 11025)
  {
    nRateIdx = 10;
  }
  else if (nSampleRate == 8000)
  {
    nRateIdx = 11;
  }
  else if (nSampleRate == 7350)
  {
    nRateIdx = 12;
  }
  
  unsigned int nNumDataBlock = nFrameLength/1024;
  nFrameLength += 7;
  
  sADTSHeader[0] = 0xFF;
  sADTSHeader[1] = 0xF1;
  sADTSHeader[2] = (nObjType<<6);
  sADTSHeader[2] |= (nRateIdx<<2);
  sADTSHeader[2] |= (nChannels&0x4)>>2;
  sADTSHeader[3] = (nChannels&0x3)<<6;
  sADTSHeader[3] |= (nFrameLength&0x1800)>>11;
  sADTSHeader[4] = (nFrameLength&0x1FF8)>>3;
  sADTSHeader[5] = (nFrameLength&0x7)<<5;
  sADTSHeader[5] |= 0x1F;
  sADTSHeader[6] = 0xFC;
  sADTSHeader[6] |= (nNumDataBlock&0x03);
}

/*
 FLV头信息 9Bytes
 第1-3字节 signature FLV 0x46 0x4c 0x56
 第4字节  version 版本号
 第5个字节 Flags 如音视频都有则为 00000101(0x05)
 第5个字节的第6个bit音频类型标志（TypeFlagsAudio）
 第5个字节的第7个bit也是保留的必须是0
 第5个字节的第8个bit视频类型标志（TypeFlagsVideo）
 6-9 DataOffset FLV头长度 其数据为00000009.
 */
struct FLVHeader {
  uint8_t signature[3];
  uint8_t version;
  uint8_t flag;
  uint8_t headerLen[4];
};

/*
 tag 分为audio、video、script
 tag = type + data_size + timestamp + timestampExtended + stream_id + data
        1B     3B         3B          1B                  3B(总是0)    数据部分
 */
struct FLVTag {
  uint8_t tagType; //8(audio) 9(video) 18(script)
  uint8_t tagDataSize[3]; //数据长度
  uint8_t tagTimeStamp[3]; //时间戳
  uint8_t tagTimeStampExtended;
  uint8_t tagStreamId[3]; //
  uint8_t* tagData;
  
public:
  FLVTag(){
    tagType = 0;
    memset(tagDataSize, 0, sizeof(uint8_t) * 3);
    memset(tagTimeStamp, 0, sizeof(uint8_t) * 3);
    tagTimeStampExtended = 0;
    memset(tagStreamId, 0, sizeof(uint8_t) * 3);
    tagData = nullptr;
  };
};

//AMF Data
struct AMFData {
  uint8_t type; //AMF数据类型
  uint32_t dataLen;//不同的类型长度所占字节数不等。 如string为U16 longString为U32
  uint8_t* data;
};

struct SequenceParameterSets {
  ushort len;
  uint8_t* data;
};

//看具体标准
struct AVCDecoderConfigurationRecord{
  uint8_t configuationVersion;
  uint8_t AVCProfileIndication;
  uint8_t profile_compatibility;
  uint8_t reserved : 6;
  uint8_t lengthSizeMinusOne:2;
  uint8_t reserved2 : 3;
  uint8_t numOfSequenceParameterSets:5;
  uint8_t numOfPictureParameterSets;
  //如果有多个sps，这里应该有多个sps结构体，简单起见，暂时先处理一个
  SequenceParameterSets sps;
  SequenceParameterSets pps;
};

//ISO 49916-3
/* 这个结构体太复杂，adts我们只需要知道audioObjectType就够了
struct AudioSpecificConfig
{
  uint8_t audioObjectType : 5;
  uint8_t samplingFrequencyIndex : 4;
  if ( samplingFrequencyIndex==0xf )
    samplingFrequency; 24 uimsbf
  channelConfiguration; 4 bslbf
  if ( audioObjectType == 1 || audioObjectType == 2 ||
      audioObjectType == 3 || audioObjectType == 4 ||
      audioObjectType == 6 || audioObjectType == 7 )
    GASpecificConfig();
  if ( audioObjectType == 8 )
    CelpSpecificConfig();
  if ( audioObjectType == 9 )
    HvxcSpecificConfig();
  if ( audioObjectType == 12 )
    TTSSpecificConfig();
  if ( audioObjectType == 13 || audioObjectType == 14 ||
      audioObjectType == 15 || audioObjectType==16)
    StructuredAudioSpecificConfig();
  if ( audioObjectType == 17 || audioObjectType == 19 ||
      audioObjectType == 20 || audioObjectType == 21 ||
      audioObjectType == 22 || audioObjectType == 23 )
    GASpecificConfig();
  if ( audioObjectType == 24)
    ErrorResilientCelpSpecificConfig();
  if ( audioObjectType == 25)
    ErrorResilientHvxcSpecificConfig();
  if ( audioObjectType == 26 || audioObjectType == 27)
    ParametricSpecificConfig();
  if ( audioObjectType == 17 || audioObjectType == 19 ||
      audioObjectType == 20 || audioObjectType == 21 ||
      audioObjectType == 22 || audioObjectType == 23 ||
      audioObjectType == 24 || audioObjectType == 25 ||
      audioObjectType == 26 || audioObjectType == 27 ) {
    epConfig; 2 bslbf
    if ( epConfig == 2 || epConfig == 3 ) {
      ErrorProtectionSpecificConfig();
    }
    if ( epConfig == 3 ) {
      directMapping; 1 bslbf
      if ( ! directMapping ) {
        //tbd
      }
    }
  }
}
*/
//AudioTagData = AudioTagDataHeader + AudioTagDataBody
struct AudioTagDataHeader{
  /*
   Format of SoundData. The following values are defined:
   0 = Linear PCM, platform endian
   1 = ADPCM
   2 = MP3
   3 = Linear PCM, little endian
   4 = Nellymoser 16 kHz mono
   5 = Nellymoser 8 kHz mono
   6 = Nellymoser
   7 = G.711 A-law logarithmic PCM
   8 = G.711 mu-law logarithmic PCM
   9 = reserved
   10 = AAC
   11 = Speex
   14 = MP3 8 kHz
   15 = Device-specific sound
   Formats 7, 8, 14, and 15 are reserved.
   AAC is supported in Flash Player 9,0,115,0 and higher.
   Speex is supported in Flash Player 10 and higher.
   */
  uint8_t soundFormat:4;
  /*
   Sampling rate. The following values are defined:
   0 = 5.5 kHz
   1 = 11 kHz
   2 = 22 kHz
   3 = 44 kHz
   */
  uint8_t soundRate:2;
  /*
   Size of each audio sample. This parameter only pertains to
   uncompressed formats. Compressed formats always decode
   to 16 bits internally.
   0 = 8-bit samples
   1 = 16-bit samples
   */
  uint8_t soundSize:1;
  /*
   Mono or stereo sound
   0 = Mono sound
   1 = Stereo sound
   */
  uint8_t soundType:1;
  /*
   if SoundFormat == 10
   The following values are defined:
   0 = AAC sequence header
   1 = AAC raw
   */
  uint8_t aacPacketType;
};

//VideoTagData = VideoTagDataHeader + VideoTagDataBody
//videoTagDataBody = NALU_Len + NALU_Data
struct VideoTagDataHeader {
  uint8_t frameType : 4;
  uint8_t codecID : 4;
  uint8_t avcPacketType;
  uint8_t compositionTime[3];
};

int main()
{
  cout.setf(ios::fixed, ios::floatfield);
  cout << "VideoTagDataHeader size: " << sizeof(VideoTagDataHeader) << endl;
  cout << "AudioTagDataHeader size: " << sizeof(AudioTagDataHeader) << endl;
  cout << "for test : " << endl;
  uint8_t testData[2] = {0xAF, 0x01};
  AudioTagDataHeader atdh;
  memcpy(&atdh, testData, sizeof(testData)/sizeof(uint8_t));
  atdh.soundFormat = testData[0]>>4;
  atdh.soundRate = testData[0] & 0x0B;
  atdh.soundSize = testData[0] & 0x02;
  atdh.soundType = testData[0] & 0x01;
  cout << "this is a flv parser" << endl;
  FLVHeader flvHeader;
  memset(&flvHeader, 0, sizeof(flvHeader));
  std::string strInputFile = "../../test3.flv";
  FILE* fp = fopen(strInputFile.c_str(), "rb");
  if(fp) {
    //输出h264码流
    std::string strOutput264 = "../../output.264";
    FILE* fpH264 = fopen(strOutput264.c_str(), "wb");
    
    //输出aac数据
    std::string strOutputAAC = "../../output.aac";
    FILE* fpAAC = fopen(strOutputAAC.c_str(), "wb");
    
    uint8_t nalPrefix[4] = {0x00, 0x00, 0x00, 0x01};
    uint8_t adts_fix_header[7] = {0};
    int nAudioObjectType = 0;
    int samplingFrequencyIndex = 0;
    FLVTag tmpTag;
    int nReadUnit = 200*1024;
    uint8_t* pReadUnit = new uint8_t[nReadUnit];
    //file size
    fseek(fp, 0, SEEK_END);
    size_t fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    cout << strInputFile << " size is " << fileSize << endl;
    
    if(nReadUnit > fileSize) {
      nReadUnit = fileSize;
    }
    size_t nRead = fread(pReadUnit, 1, nReadUnit, fp);
    cout << "read size: " << nRead << endl;
    if(nRead >= 9) {
      memcpy(&flvHeader, pReadUnit, sizeof(flvHeader));
      cout << "FLVHeader: " << char(flvHeader.signature[0]) <<
      " " << char(flvHeader.signature[1]) << " " << char(flvHeader.signature[2])<<endl;
      cout << "version: " << flvHeader.version << endl;
      cout << "HasAudio: " << string((flvHeader.flag & 0x04) ? "true" : "false") << endl;
      cout << "HasVideo: " << string((flvHeader.flag & 0x01) ? "true" : "false") << endl;
      cout << "flvHeader len: " << (flvHeader.headerLen[0] << 24 | flvHeader.headerLen[1] << 16 | flvHeader.headerLen[2] << 8 | flvHeader.headerLen[3]) << endl;
    }
    fileSize -= nRead;
    
    int nReadSize = 0;
    int nBufLen = 0;
    cout << "FLV Body" << endl;
    int nAudioTagCnt=0, nVideoTagCnt=0;
    FLVTag tagN;
    uint nOffset = sizeof(FLVHeader) + 4; // +4 是因为前一个tag长度占4个字节
    AVCDecoderConfigurationRecord avcDCR;
    while (fileSize > 0 || nOffset < nReadSize) {
      //read tag
      //判断 pReadUnit left size是否够读
      if (nReadUnit - nOffset < 11) {
        memmove(pReadUnit, pReadUnit+nOffset, nReadUnit-nOffset);
        if(fileSize < nReadUnit) {
          nReadSize = fileSize;
        } else {
          nReadSize = nOffset;
        }
        fread(pReadUnit+nReadUnit-nOffset, 1, nReadSize, fp);
        fileSize -= nReadSize;
        nReadSize += (nReadUnit-nOffset);
        nOffset = 0;
      }
      tagN.tagType = pReadUnit[nOffset];
      nOffset += 1;
      memcpy(tagN.tagDataSize, pReadUnit+nOffset, sizeof(tagN.tagDataSize));
      nOffset += sizeof(tagN.tagDataSize);
      int tagDataLen = (tagN.tagDataSize[0] << 16) | (tagN.tagDataSize[1] << 8) | (tagN.tagDataSize[2]);
      //判断读取的数据是否够tagSize
      if (nReadUnit - nOffset < tagDataLen) {
        memmove(pReadUnit, pReadUnit+nOffset, nReadUnit-nOffset);
        nReadSize = nOffset;
        if(fileSize < nReadUnit) {
          nReadSize = fileSize;
        } else {
          nReadSize = nOffset;
        }
        fread(pReadUnit+nReadUnit-nOffset, 1, nReadSize, fp);
        fileSize -= nReadSize;
        nReadSize += (nReadUnit-nOffset);
        nOffset = 0;
      }
      
      memcpy(tagN.tagTimeStamp, pReadUnit+nOffset, sizeof(tagN.tagTimeStamp));
      nOffset += sizeof(tagN.tagTimeStamp);
      memcpy(&tagN.tagTimeStampExtended, pReadUnit+nOffset, sizeof(tagN.tagTimeStampExtended));
      nOffset += sizeof(tagN.tagTimeStampExtended);
      memcpy(tagN.tagStreamId, pReadUnit+nOffset, sizeof(tagN.tagStreamId));
      nOffset += sizeof(tagN.tagStreamId);
      if(tagN.tagData) {
        delete[] tagN.tagData;
        tagN.tagData = nullptr;
      }
      tagN.tagData = new uint8_t[tagDataLen];
      memcpy(tagN.tagData, pReadUnit+nOffset, tagDataLen);
      nOffset += tagDataLen;
      cout << "tag: " << endl;
      std::string strType = "script";
      if(tagN.tagType == 0x08) {
        strType = "audio_" + std::to_string(nAudioTagCnt);
      } else if (tagN.tagType == 0x09) {
        strType = "video_" + std::to_string(nVideoTagCnt);
      }
      printf("type: %02x %s\n", tagN.tagType, strType.c_str());
      cout << " data size: " << tagDataLen << endl;
      cout << " timeStamp: " << (tagN.tagTimeStamp[0] << 16 | tagN.tagTimeStamp[1] << 8 | tagN.tagTimeStamp[2]) << endl;
      cout << " timeStampExtended: " << int(tagN.tagTimeStampExtended) << endl;
      cout << " streamId: " << (tagN.tagStreamId[0] << 16 | tagN.tagStreamId[1] << 8 | tagN.tagStreamId[2])  << endl;
      if (tagN.tagType == 0x12) {
        //script data
        //解析AMF type 2:string 后面两个字节为字符串长度
        //第一个一般是onMetaData
        int tagDataOffset = 0;
        int nAMFCnt = 0;
        while(tagDataOffset < tagDataLen) {
          if(tagN.tagData[tagDataOffset] == 0x02)
          {
            nAMFCnt++;
            cout << "AMF" << nAMFCnt << endl;
            //字符串
            tagDataOffset += 1;
            int nStringLen = tagN.tagData[tagDataOffset] << 16 | tagN.tagData[tagDataOffset+1];
            tagDataOffset += 2;
            cout << "tag string len: " << nStringLen << endl;
            string strContent;
            for(int i = 0; i < nStringLen;i++)
            {
              strContent.push_back(tagN.tagData[i+tagDataOffset]);
            }
            cout << "tag string content: " << strContent << endl;
            tagDataOffset += nStringLen;
          }
          else if (tagN.tagData[tagDataOffset] == 0x08)
          {
           //数组 UI32(数组个数)
            nAMFCnt++;
            cout << "AMF" << nAMFCnt << endl;
            tagDataOffset+=1;
            int nArrayCnt = (tagN.tagData[tagDataOffset] << 24) | (tagN.tagData[tagDataOffset+1] << 16) | (tagN.tagData[tagDataOffset+2] << 8) | (tagN.tagData[tagDataOffset+3]);
            cout << "array size: " << nArrayCnt << endl;
            tagDataOffset += 4;
            for(auto i = 0; i < nArrayCnt; i++)
            {
              //ScriptDataString+ScriptDataValue
              //U16+内容   U8(类型) 根据类型去解析,如0=NUMBER=DOUBLE 8字节等
              int nDataStringLen = tagN.tagData[tagDataOffset] << 8 | tagN.tagData[tagDataOffset+1];
              tagDataOffset += 2;
              std::string strDataKey = "";
              for(int j=0; j<nDataStringLen; j++) {
                strDataKey.push_back(tagN.tagData[tagDataOffset+j]);
              }
              cout << "metaData property, " << strDataKey;
              tagDataOffset += nDataStringLen;
              //解析scriptDataValue
              uint8_t dataValueType = tagN.tagData[tagDataOffset];
              tagDataOffset += 1;
              switch (dataValueType) {
                case 0://Number
                {
                  double dValue = 0.0;
                  char szBuf[8];
                  memcpy((void*)szBuf, tagN.tagData + tagDataOffset, sizeof(double));
                  for(int k = 0; k < 8/2; k++) {
                    swap(szBuf[k], szBuf[7-k]);
                  }
                  memcpy(&dValue, szBuf, sizeof(double));
                  cout << ": " << dValue << endl;
                  tagDataOffset += 8;
                  if(strDataKey.compare("audiosamplerate") == 0) {
                    //从audioTagData中取到的不准
                    samplingFrequencyIndex = dValue;
                  }
                }
                  break;
                case 1://Bool
                {
                  if(tagN.tagData[tagDataOffset] == 0) {
                    cout << " false" << endl;
                  } else {
                    cout << " true" << endl;
                  }
                  tagDataOffset += 1;
                }
                  break;
                case 2://String
                {
                  int nStrLen = tagN.tagData[tagDataOffset] << 8 | tagN.tagData[tagDataOffset+1];
                  tagDataOffset += 2;
                  string strContent="";
                  for (int i = 0; i < nStrLen; i++) {
                    strContent.push_back(tagN.tagData[tagDataOffset+i]);
                  }
                  cout << ": " << strContent << endl;
                  tagDataOffset += nStrLen;
                }
                  break;
                default:
                  break;
              }
            }
            //数组结束标识 00 00 09
            printf("array end marker: %02x %02x %02x\n", tagN.tagData[tagDataOffset], tagN.tagData[tagDataOffset+1], tagN.tagData[tagDataOffset+2]);
            tagDataOffset += 3;
          }
        }
      } else if (tagN.tagType == 0x08) {
        //audio data
        nAudioTagCnt++;
        //
        AudioTagDataHeader atagDataHeader;
        atagDataHeader.soundFormat = tagN.tagData[0] >> 4;
        atagDataHeader.soundRate = tagN.tagData[0] & 0x0B;
        atagDataHeader.soundSize = tagN.tagData[0] & 0x02;
        atagDataHeader.soundType = tagN.tagData[0] & 0x01;
        atagDataHeader.aacPacketType = tagN.tagData[1];
        int nAudioTagDataOffset = sizeof(AudioTagDataHeader);
        if((atagDataHeader.soundFormat&0x0a) && (atagDataHeader.aacPacketType & 0x01)) {
          //aac raw
          int nChannelType = 1;
          if(atagDataHeader.soundType & 0x01) {
            nChannelType = 2;
          }
          int nSampleRate = 5500;
          if(atagDataHeader.soundRate & 0x00) {
            nSampleRate = 5500;
          } else if (atagDataHeader.soundRate & 0x03) {
            nSampleRate = 44000;
          } else if(atagDataHeader.soundRate & 0x01) {
            nSampleRate = 11000;
          } else if(atagDataHeader.soundRate & 0x02) {
            nSampleRate = 22000;
          }
          //nSampleRate 不准确
          FormatATDSHeader(nAudioObjectType - 1, tagDataLen-nAudioTagDataOffset, nChannelType, samplingFrequencyIndex, (uint8_t*)adts_fix_header);
          if(fpAAC){
            fwrite(adts_fix_header, 1, sizeof(adts_fix_header)/sizeof(uint8_t), fpAAC);
            fwrite(tagN.tagData+nAudioTagDataOffset, 1, tagDataLen-nAudioTagDataOffset, fpAAC);
          }
        } else {
          //aac sequence header
          //获取aac audioObjectType 详细见ISO14496-3 Table 1.1
          // 0-null 1-AAC_Main 2-AAC_LC 3-AAC_SSR
          nAudioObjectType = tagN.tagData[nAudioTagDataOffset] >> 3;
          if(samplingFrequencyIndex == 0) {
            samplingFrequencyIndex = (tagN.tagData[nAudioTagDataOffset] & 0x07) << 3 | (tagN.tagData[nAudioTagDataOffset+1]>>7);
          }
        }
      } else if (tagN.tagType == 0x09) {
        //video data
        nVideoTagCnt++;
        //需要对videoTagData做进一步解析，一般第一个tagData中包含了sps pps等信息,参考结构体AVCDecoderConfigurationRecord
        VideoTagDataHeader vtagDataHeader;
        memcpy(&vtagDataHeader, tagN.tagData, sizeof(vtagDataHeader));
        vtagDataHeader.frameType = tagN.tagData[0] >> 4;
        vtagDataHeader.codecID = tagN.tagData[0] & 0x0F;
        printf("tag frameType: %02x, codecID(7-AVC): %02x, AVCPacketType: %02x\n",
               vtagDataHeader.frameType, vtagDataHeader.codecID, vtagDataHeader.avcPacketType);
        if(vtagDataHeader.frameType == 0x01) {
          //KeyFrame(for avc, a seekable frame)
          if(vtagDataHeader.avcPacketType == 0x00) {
            //AVCDecodeConfigurationRecord
            int nOffset = sizeof(vtagDataHeader);
            memcpy(&avcDCR, tagN.tagData+nOffset, 5);
            nOffset += 5;
            avcDCR.reserved2 = tagN.tagData[nOffset] & 0x70;
            avcDCR.numOfSequenceParameterSets = tagN.tagData[nOffset] & 0x1F;
            printf("lengthSizeMinusOne : %02x\n", avcDCR.lengthSizeMinusOne);
            
            printf("sps cnt : %02x\n", avcDCR.numOfSequenceParameterSets);
            nOffset += 1;
            avcDCR.sps.len = tagN.tagData[nOffset] << 8 | tagN.tagData[nOffset+1];
            nOffset += 2;
            avcDCR.sps.data = new uint8_t[avcDCR.sps.len];
            memcpy(avcDCR.sps.data, tagN.tagData+nOffset, avcDCR.sps.len);
            nOffset += avcDCR.sps.len;
            avcDCR.numOfPictureParameterSets = tagN.tagData[nOffset];
            printf("pps cnt : %02x\n", avcDCR.numOfPictureParameterSets);
            nOffset += 1;
            avcDCR.pps.len = tagN.tagData[nOffset] << 8 | tagN.tagData[nOffset+1];
            nOffset += 2;
            avcDCR.pps.data = new uint8_t[avcDCR.pps.len];
            memcpy(avcDCR.pps.data, tagN.tagData+nOffset, avcDCR.pps.len);
            nOffset += avcDCR.pps.len;
            
            if(tagN.tagData[sizeof(vtagDataHeader) + 4] & 0x1F)
            {
              //key frame
              fwrite(nalPrefix, 1, sizeof(nalPrefix)/sizeof(uint8_t), fpH264);
              fwrite(avcDCR.sps.data, 1, avcDCR.sps.len, fpH264);
              fwrite(nalPrefix, 1, sizeof(nalPrefix)/sizeof(uint8_t), fpH264);
              fwrite(avcDCR.pps.data, 1, avcDCR.pps.len, fpH264);
            }
          }
          else if (vtagDataHeader.avcPacketType == 0x01)
          {
            //one or more nalu
          }
        }
        if(vtagDataHeader.avcPacketType == 0x02) {
          cout << "avcPacketType == 0x02" << endl;
        }
        if(fpH264 && vtagDataHeader.avcPacketType == 0x01)
        {
          //sps pps 写一次跟写多次都可以，如果sps pps中间有变化，必须要写多次。
//          if(tagN.tagData[sizeof(vtagData) + 4] & 0x1F)
//          {
//            //key frame
//            fwrite(nalPrefix, 1, sizeof(nalPrefix)/sizeof(uint8_t), fpH264);
//            fwrite(avcDCR.sps.data, 1, avcDCR.sps.len, fpH264);
//            fwrite(nalPrefix, 1, sizeof(nalPrefix)/sizeof(uint8_t), fpH264);
//            fwrite(avcDCR.pps.data, 1, avcDCR.pps.len, fpH264);
//          }
          //data中开头4字节为nalu的长度，应该略去
          int tagDataOffset = sizeof(vtagDataHeader);
          uint8_t naluSize[4] = {0};
          //多个nalu
          do {
            int naluLen = tagN.tagData[tagDataOffset] << 24 | tagN.tagData[tagDataOffset+1] << 16 | tagN.tagData[tagDataOffset+2] << 8 | tagN.tagData[tagDataOffset+3];
            tagDataOffset+=4;
            fwrite(nalPrefix, 1, sizeof(nalPrefix)/sizeof(uint8_t), fpH264);
            fwrite(tagN.tagData + tagDataOffset, 1, naluLen, fpH264);
            tagDataOffset += naluLen;
          } while (tagDataOffset < tagDataLen);
        }
      }
      else {
        cout << "get error tag type:" << tagN.tagType << endl;
        break;
      }
      uint nPreBlockSize = pReadUnit[nOffset] << 24 | pReadUnit[nOffset+1] << 16 | pReadUnit[nOffset+2] << 8 | pReadUnit[nOffset+3];
      nOffset += 4;
      cout << "pre block size: " << nPreBlockSize << endl;
    }
    cout << "now size: " << ftell(fp) << endl;
    
    if (fpH264) {
      fclose(fpH264);
    }
    if(fpAAC) {
      fclose(fpAAC);
    }
    delete[] tmpTag.tagData;
    delete[] pReadUnit;
    fclose(fp);
    fp = nullptr;
    
    cout << "video tag: " << nVideoTagCnt << endl;
    cout << "audio tag: " << nAudioTagCnt << endl;
  }
  else {
    cout << "file " << strInputFile << "cannot open" << endl;
  }
  return 0;
}
