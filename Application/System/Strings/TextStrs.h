//-----------------------------------------------------------------------------
/*
 File        : TextStrs.h
 Version     : V1.01
 By          : 银网科技

 Description :按语言类型选择资源文件

 Date       : 2017.9.10
*/
//-----------------------------------------------------------------------------
#ifndef DEV_TEXT_STRS_H
#define DEV_TEXT_STRS_H

#include "Dev_Cfg.h"
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
// 全局宏
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define idDevFuncUVTC         0x1000    //

#define idDevCopyright        0x1008    //


// 封面
#define idGPDevFamiry         0x1010    //
#define idGPRouterName        0x1011    //
// 定值区
#define idLabHBlockNo         0x1020    //
#define idHintBlockNo         0x1021    //

#define idStatusLocal         0x1028    //
#define idStatusRemote        0x1029    //
#define idStatusSpCharging    0x102A    //
#define idStatusSpCharged     0x102B    //

// 界面语言
#define idLANG_CHS            0x1030    //
#define idLANG_ENG            0x1031    //

// 经销商列表
#define idVedor01             0x1050    //
#define idCulture01           0x1051    //
#define idVedor02             0x1052    //
#define idCulture02           0x1053    //
#define idVedor03             0x1054    //
#define idCulture03           0x1055    //

// 常用按键
#define idBtnCapOk            0x1060    //
#define idBtnCapCancel        0x1061    //
#define idBtnCapClose         0x1062    //
#define idBtnCapYes           0x1063    //
#define idBtnCapNo            0x1064    //
#define idBtnCapBegin         0x1065    //

#define idEnableYes           0x1068    //
#define idEnableNo            0x1069    //

// 系统工作状态
#define idSysMode0            0x1080    // 正常工作
#define idSysMode1            0x1081    // 正在起动
#define idSysMode2            0x1082    // 校准状态
#define idSysMode3            0x1083    // 传动试验
#define idSysMode4            0x1084    // 正在维护参数
#define idSysMode5            0x1085    // 软件故障
#define idSysMode6            0x1086    // 硬件故障
#define idSysMode7            0x1087    // 功能类型错误
#define idSysMode8            0x1088    // 定值错误
#define idSysModeX            0x1089    // 未知状态

// 系统出错信息
#define idDevFault01          0x1300    //
#define idDevFault02          0x1301    //
#define idDevFault03          0x1302    //
#define idDevFault04          0x1303    //
#define idDevFault05          0x1304    //
#define idDevFault06          0x1305    //
#define idDevFault07          0x1306    //
#define idDevFault08          0x1307    //
#define idDevFault09          0x1308    //
#define idDevFault10          0x1309    //

// 装置故障
#define iaCriticalInfo        0x1310    // 装置故障提示
#define iaCriticalFW          0x1311    //
#define iaCriticalHW          0x1312    //
#define iaCriticalFT          0x1313    //
#define iaCriticalDZ          0x1314    //
#define iaCriticalUKN         0x1315    //

#define iaCriticalFmt         0x1320    //
#define iaCriticalRst         0x1321    //
#define iaCriticalOk          0x1328    //
#define iaCriticalFailed      0x1329    //

// 主窗体信息
#define idMainLabel01         0x1501
#define idMainLabel02         0x1502
#define idMainLabel03         0x1503
#define idMainLabel04         0x1504
#define idMainLabel05         0x1505
#define idMainLabel06         0x1506
#define idMainLabel07         0x1507
#define idMainLabel08         0x1508
#define idMainLabel09         0x1509

#define idMainStat01          0x1571
#define idMainStat02          0x1572
#define idMainStat03          0x1573
#define idMainStat04          0x1574
#define idMainStat05          0x1575
#define idMainStat06          0x1576
#define idMainStat07          0x1577
#define idMainStat08          0x1578
#define idMainStat09          0x1579

#define idMainFmSt01          0x1580
#define idMainFmSt02          0x1581
#define idMainFmSt03          0x1582
#define idMainFmSt04          0x1583
#define idMainFmSt05          0x1584


// 主菜单
#define idMenuCaption         0x1800    //

#define idMenuName1           0x1810    //
#define idMenuDesp1           0x1811    //
#define idMenuName2           0x1812    //
#define idMenuDesp2           0x1813    //
#define idMenuName3           0x1814    //
#define idMenuDesp3           0x1815    //
#define idMenuName4           0x1816    //
#define idMenuDesp4           0x1817    //
#define idMenuName5           0x1818    //
#define idMenuDesp5           0x1819    //
#define idMenuName6           0x181A    //
#define idMenuDesp6           0x181B    //
#define idMenuName7           0x181C    //
#define idMenuDesp7           0x181D    //
#define idMenuName8           0x181E    //
#define idMenuDesp8           0x181F    //
#define idMenuName9           0x1820    //
#define idMenuDesp9           0x1821    //
#define idMenuName10          0x1822    //
#define idMenuDesp10          0x1823    //

//#define idMenuName1           0x1810    //
//#define idMenuDesp1           0x1811    //
//#define idMenuName1_1         0x1812    //
//#define idMenuDesp1_1         0x1813    //
//#define idMenuName1_2         0x1814    //
//#define idMenuDesp1_2         0x1815    //
//#define idMenuName1_3         0x1816    //
//#define idMenuDesp1_3         0x1817    //
//#define idMenuName1_4         0x1818    //
//#define idMenuDesp1_4         0x1819    //
//#define idMenuName1_5         0x181A    //
//#define idMenuDesp1_5         0x181B    //
//#define idMenuName1_6         0x181C    //
//#define idMenuDesp1_6         0x181D    //
//#define idMenuName1_7         0x181E    //
//#define idMenuDesp1_7         0x181F    //

//#define idMenuName2           0x1820    //
//#define idMenuDesp2           0x1821    //
//#define idMenuName2_1         0x1822    //
//#define idMenuDesp2_1         0x1823    //
//#define idMenuName2_2         0x1824    //
//#define idMenuDesp2_2         0x1825    //
//#define idMenuName2_3         0x1826    //
//#define idMenuDesp2_3         0x1827    //
//#define idMenuName2_4         0x1828    //
//#define idMenuDesp2_4         0x1829    //
//#define idMenuName2_5         0x182A    //
//#define idMenuDesp2_5         0x182B    //
//#define idMenuName2_6         0x182C    //
//#define idMenuDesp2_6         0x182D    //
//#define idMenuName2_7         0x182E    //
//#define idMenuDesp2_7         0x182F    //

//#define idMenuName3           0x1830    //
//#define idMenuDesp3           0x1831    //
//#define idMenuName3_1         0x1832    //
//#define idMenuDesp3_1         0x1833    //
//#define idMenuName3_2         0x1834    //
//#define idMenuDesp3_2         0x1835    //
//#define idMenuName3_3         0x1836    //
//#define idMenuDesp3_3         0x1837    //
//#define idMenuName3_4         0x1838    //
//#define idMenuDesp3_4         0x1839    //
//#define idMenuName3_5         0x183A    //
//#define idMenuDesp3_5         0x183B    //
//#define idMenuName3_6         0x183C    //
//#define idMenuDesp3_6         0x183D    //
//#define idMenuName3_7         0x183E    //
//#define idMenuDesp3_7         0x183F    //
//#define idMenuName3_8         0x1840    //
//#define idMenuDesp3_8         0x1841    //

//#define idMenuName4           0x1850    //
//#define idMenuDesp4           0x1851    //
//#define idMenuName4_1         0x1852    //
//#define idMenuDesp4_1         0x1853    //
//#define idMenuName4_1a        0x1854    //
//#define idMenuDesp4_1a        0x1855    //
//#define idMenuName4_2         0x1856    //
//#define idMenuDesp4_2         0x1857    //
//#define idMenuName4_2a        0x1858    //
//#define idMenuDesp4_2a        0x1859    //
//#define idMenuName4_3         0x185A    //
//#define idMenuDesp4_3         0x185B    //
//#define idMenuName4_3a        0x185C    //
//#define idMenuDesp4_3a        0x185D    //
//#define idMenuName4_4         0x185E    //
//#define idMenuDesp4_4         0x185F    //
//#define idMenuName4_4a        0x1860    //
//#define idMenuDesp4_4a        0x1861    //
//#define idMenuName4_5         0x1862    //
//#define idMenuDesp4_5         0x1863    //
//#define idMenuName4_5a        0x1864    //
//#define idMenuDesp4_5a        0x1865    //
//#define idMenuName4_6         0x1866    //
//#define idMenuDesp4_6         0x1867    //
//#define idMenuName4_6a        0x1868    //
//#define idMenuDesp4_6a        0x1869    //
//#define idMenuName4_7         0x186A    //
//#define idMenuDesp4_7         0x186B    //
//#define idMenuName4_8         0x186C    //
//#define idMenuDesp4_8         0x186D    //
//#define idMenuName4_9         0x186E    //
//#define idMenuDesp4_9         0x186F    //
//#define idMenuName4_10        0x1870    //
//#define idMenuDesp4_10        0x1871    //
//#define idMenuName4_11        0x1872    //
//#define idMenuDesp4_11        0x1873    //

//#define idMenuName5           0x1880    //
//#define idMenuDesp5           0x1881    //

//#define idMenuName6           0x1890    //
//#define idMenuDesp6           0x1891    //
//#define idMenuName6_1         0x1892    //
//#define idMenuDesp6_1         0x1893    //
//#define idMenuName6_2         0x1894    //
//#define idMenuDesp6_2         0x1895    //
//#define idMenuName6_3         0x1896    //
//#define idMenuDesp6_3         0x1897    //
//#define idMenuName6_4         0x1898    //
//#define idMenuDesp6_4         0x1899    //
//#define idMenuName6_5         0x189A    //
//#define idMenuDesp6_5         0x189B    //

//#define idMeasAdjuest         0x18A0    //
//#define idMenuName7           0x18A1    //
//#define idMenuDesp7           0x18A2    //
//#define idMenuName7_1         0x18A3    //
//#define idMenuDesp7_1         0x18A4    //
//#define idMenuName7_2         0x18A5    //
//#define idMenuDesp7_2         0x18A6    //
//#define idMenuName7_3         0x18A7    //
//#define idMenuDesp7_3         0x18A8    //
//#define idMenuName7_4         0x18A9    //
//#define idMenuDesp7_4         0x18AA    //
//#define idMenuName7_5         0x18AB    //
//#define idMenuDesp7_5         0x18AC    //
//#define idMenuName7_6         0x18AD    //
//#define idMenuDesp7_6         0x18AE    //
//#define idMenuName7_7         0x18AF    //
//#define idMenuDesp7_7         0x18B0    //
//#define idMenuName7_8         0x18B1    //
//#define idMenuDesp7_8         0x18B2    //
//#define idMenuName7_0         0x18BA    //
//#define idMenuDesp7_0         0x18BB    //

//#define idMenuName8           0x18C0    //
//#define idMenuDesp8           0x18C1    //
//#define idMenuName8_1         0x18C2    //
//#define idMenuDesp8_1         0x18C3    //
//#define idMenuName8_2         0x18C4    //
//#define idMenuDesp8_2         0x18C5    //

//#define idMenuName9           0x18D1    //
//#define idMenuDesp9           0x18D2    //

//// 工厂菜单
//#define idMenuName0           0x18F0    //
//#define idMenuDesp0           0x18F1    //
//#define idMenuName0_1         0x18F2    //
//#define idMenuDesp0_1         0x18F3    //
//#define idMenuName0_2         0x18F4    //
//#define idMenuDesp0_2         0x18F5    //
//#define idMenuName0_3         0x18F6    //
//#define idMenuDesp0_3         0x18F7    //
//#define idMenuName0_4         0x18F8    //
//#define idMenuDesp0_4         0x18F9    //
//#define idMenuName0_5         0x18FA    //
//#define idMenuDesp0_5         0x18FB    //

// 恢复默认值
#define idMenuRstSetting      0x1900    //
#define idMenuRstSetCfgOk     0x1901    //
#define idMenuRstSetCfgErr    0x1902    //
#define idMenuRstDevCfg       0x1903    //
#define idMenuRstDevCfgOk     0x1904    //
#define idMenuRstDevCfgErr    0x1905    //
// 菜单内提示信息
#define idFuncLocked          0x1910    //
#define idFuncFailed          0x1911    //
#define idFuncUnanth          0x1912    //
// 清除事件记录 确认
#define idClrEventLogCfm      0x1920    //
#define idClrAlarmLogCfm      0x1921    //
#define idClrFaultLogCfm      0x1922    //
#define idClrEventLogOk       0x1923    //
#define idClrAlarmLogOk       0x1924    //
#define idClrFaultLogOk       0x1925    //
#define idClrWaveLogCfm       0x1926    //
#define idClrWaveLogOk        0x1927    //
#define idClrWaveLogFail      0x1928    //
#define idClrMeteringCfm      0x1929    //
#define idClrMeteringOk       0x192A    //
#define idClrMeteringFail     0x192B    //

// 重启装置确认
#define idDeviceResetCfm      0x192F    //

// 用户登录窗体
#define idLoginCap1           0x1940    //
#define idLoginCap2           0x1941    //
#define idLoginCap3           0x1942    //
#define idLoginApply          0x1943    //
#define idLoginChkErrCap      0x1944    //
#define idLoginChkErrTxt      0x1945    //
#define idSetPSWDCap1         0x1946    //
#define idSetPSWDCap2         0x1947    //
#define idSetPSWDCFM          0x1948    //
#define idSetPSWDCFM1         0x1949    //
#define idSetPSWDCFM2         0x194A    //
#define idSetPSWDOk           0x194B    //
#define idSetPSWDOk1          0x194C    //
#define idSetPSWDOk2          0x194D    //
#define idSetPSWDOk3          0x194E    //

//// 切换运行区窗体
//#define idSSFmCurBlock        0x1960    //
//#define idSSFmCaption1        0x1961    //
//#define idSSFmCaption2        0x1962    //
//#define idSSFmCaption3        0x1963    //
//#define idSSFmCaption4        0x1964    //
//#define idSSDestIsCur         0x1965    //
//#define idSSSwtAtvHldOk       0x1966    //
//#define idSSSwtAtvHldErr      0x1967    //
//#define idSSSwtEdtHldErr      0x1968    //
//#define idSSSwtCpyHldOk       0x1969    //
//#define idSSSwtCpyHldErr      0x196A    //

//// 恢复默认定值
//#define idSetModifiedCap      0x1970    //
//#define idSetModifiedLbl      0x1971    //
//#define idSetSavedOk          0x1972    //

// 配置日期时间
#define idSetDateTimeCap      0x1980    //
#define idSetClockOk          0x1981    //

//// 手动分合
//#define idManuCtrlOp0         0x1990
//#define idManuCtrlOp1         0x1991
//#define idManuCtrlInf0a       0x1992
//#define idManuCtrlInf0b       0x1993
//#define idManuCtrlInf1        0x1994
//#define idManuCtrlInf2        0x1995
//#define idManuCtrlInf3        0x1996

//// 校准提示
//#define idACCalibHint         0x19A0    //
//#define idDCCalibHint0        0x19A1    //
//#define idDCCalibHint1        0x19A2    //
//#define idCalibCap            0x19A8    //
//#define idCalibOk             0x19A9    //
//#define idCalibSomeOk         0x19AA    //
//#define idCalibFailed         0x19AB    //
//#define idMeaAdjFailed        0x19AC    //

// Data list group
#define idDVGroup01           0x1A40
#define idDVGroup02           0x1A41
#define idDVGroup03           0x1A42
#define idDVGroup04           0x1A43
#define idDVGroup05           0x1A44

#define idMeasName01          0x1A50
#define idMeasName02          0x1A51
#define idMeasName03          0x1A52
#define idMeasName04          0x1A53
#define idMeasName05          0x1A54
#define idMeasName06          0x1A55
#define idMeasName07          0x1A56
#define idMeasName08          0x1A57
#define idMeasName09          0x1A58
#define idMeasName10          0x1A59
#define idMeasName11          0x1A5A
#define idMeasName12          0x1A5B
#define idMeasName13          0x1A5C
#define idMeasName14          0x1A5D

// 枚举提示
#define idEnumChanged         0x1A71    //
#define idEnumChgRst          0x1A72    //

// 事件列表窗体
#define idEventCatalog1       0x1A81
#define idEventCatalog2       0x1A82
#define idEventCatalog3       0x1A83

#define idDateTimeFormat      0x1A88    //
#define idEventEmpty          0x1A89    //
#define idEvtDataEmpty        0x1A8A    //

// 设备配置
#define idDevOptName01        0x2000    //
#define idDevOptName02        0x2001    //
#define idDevOptName03        0x2002    //
#define idDevOptName04        0x2003    //

#define idVoltMode0           0x2010    //
#define idVoltMode1           0x2011    //
//主终端
#define idMain_Terminal0      0x2012
#define idMain_Terminal1      0x2013
//重合闸次数
#define idReclose_cnt0        0x2015
#define idReclose_cnt1        0x2016
#define idReclose_cnt2        0x2017
//关联方向
#define idSwitch_Direction0   0x2018
#define idSwitch_Direction1   0x2019
//运行模式
#define idRun_Mode0           0x2020
#define idRun_Mode1           0x2021
#define idRun_Mode2           0x2022
//FA运行模式
#define idFARun_Mode0         0x2023
#define idFARun_Mode1         0x2024
//重合闸次数
#define idReclose_typ0        0x2025
#define idReclose_typ1        0x2026
#define idReclose_typ2        0x2027
//开关组别
#define idSwitch_Kind0        0x2028
#define idSwitch_Kind1        0x2029
#define idSwitch_Kind2        0x202A

//// PT变比
//#define idPTRatio01           0x2031
//#define idPTRatio02           0x2032
//#define idPTRatio03           0x2033
//#define idPTRatio04           0x2034
//#define idPTRatio05           0x2035
//#define idPTRatio06           0x2036
//#define idPTRatio07           0x2037
//#define idPTRatio08           0x2038
//#define idPTRatio09           0x2039
//#define idPTRatio10           0x2030
//#define idPTRatio11           0x203A
//#define idPTRatio12           0x203B

//// CT输入
//#define idCTRatio01           0x2040
//#define idCTRatio02           0x2041

//#define idCTNum01             0x2042
//#define idCTNum02             0x2043

// 设备参数
#define idDevCfgName01        0x2100    //
#define idDevCfgName02        0x2101    //
#define idDevCfgName03        0x2102    //
#define idDevCfgName04        0x2103    //
#define idDevCfgName05        0x2104    //
#define idDevCfgName06        0x2105    //
#define idDevCfgName07        0x2106    //
#define idDevCfgName08        0x2107    //
#define idDevCfgName09        0x2108    //
#define idDevCfgName10        0x2109    //
#define idDevCfgName11        0x210A    //
#define idDevCfgName12        0x210B    //
#define idDevCfgName13        0x210C    //
#define idDevCfgName14        0x210D    //
#define idDevCfgName15        0x210E    //
#define idDevCfgName16        0x210F    //
#define idDevCfgName17        0x2110    //
#define idDevCfgName18        0x2111    //
#define idDevCfgName19        0x2112    //
#define idDevCfgName20        0x2113    //

// 日期时间
#define idDateTimeName01      0x2200    //
#define idDateTimeName02      0x2201    //
#define idDateTimeName03      0x2202    //
#define idDateTimeName04      0x2203    //
#define idDateTimeName05      0x2204    //
#define idDateTimeName06      0x2205    //
#define idDateTimeName07      0x2206    //
#define idDateTimeName08      0x2207    //
#define idDateTimeName09      0x2208    //

// 规约名称列表
//#define idProtocolName01      0x2300    //
//#define idProtocolName02      0x2301    //
//#define idProtocolName03      0x2302    //
//#define idProtocolName04      0x2303    //
//#define idProtocolName05      0x2304    //
//#define idProtocolName06      0x2305    //

// 通信口
//#define idNET1Name            0x2330    //
//#define idNET2Name            0x2331    //
//#define idNET3Name            0x2332    //
//#define idNET4Name            0x2333    //

#define idUART1Name           0x2338    //
#define idUART2Name           0x2339    //

//#define idSocket1Name         0x2340    //
//#define idSocket2Name         0x2341    //
//#define idSocket3Name         0x2342    //
//#define idSocket4Name         0x2343    //
//#define idSocket5Name         0x2344    //
//#define idSocket6Name         0x2345    //
//#define idSocket7Name         0x2346    //
//#define idSocket8Name         0x2347    //

//// 网口
//#define idNICCfgName01        0x2350    //
//#define idNICCfgName02        0x2351    //
//#define idNICCfgName03        0x2352    //
//#define idNICCfgName04        0x2353    //
//#define idNICCfgName05        0x2354    //
//#define idNICCfgName06        0x2355    //
//#define idNICCfgName07        0x2356    //
//#define idNICCfgName08        0x2357    //

//#define idNICCfg01            0x2360    // idNICNo(1) szNICCfgName01
//#define idNICCfg02            0x2361    // idNICNo(1) szNICAddr(1)
//#define idNICCfg03            0x2362    // idNICNo(1) szNICAddr(2)
//#define idNICCfg04            0x2363    // idNICNo(1) szNICAddr(3)
//#define idNICCfg05            0x2364    // idNICNo(1) szNICAddr(4)
//#define idNICCfg06            0x2365    // idNICNo(1) szNICCfgName02
//#define idNICCfg07            0x2366    // idNICNo(1) szNetMask(1)
//#define idNICCfg08            0x2367    // idNICNo(1) szNetMask(2)
//#define idNICCfg09            0x2368    // idNICNo(1) szNetMask(3)
//#define idNICCfg10            0x2369    // idNICNo(1) szNetMask(4)
//#define idNICCfg11            0x236A    // idNICNo(1) szNICCfgName03
//#define idNICCfg12            0x236B    // idNICNo(1) szNetGate(1)
//#define idNICCfg13            0x236C    // idNICNo(1) szNetGate(2)
//#define idNICCfg14            0x236D    // idNICNo(1) szNetGate(3)
//#define idNICCfg15            0x236E    // idNICNo(1) szNetGate(4)
//#define idNICCfg16            0x236F    // idNICNo(1) szNICCfgName04
//#define idNICCfg17            0x2370    // idNICNo(1) szNetMAC(1)
//#define idNICCfg18            0x2371    // idNICNo(1) szNetMAC(2)
//#define idNICCfg19            0x2372    // idNICNo(1) szNetMAC(3)
//#define idNICCfg20            0x2373    // idNICNo(1) szNetMAC(4)
//#define idNICCfg21            0x2374    // idNICNo(1) szNetMAC(5)
//#define idNICCfg22            0x2375    // idNICNo(1) szNetMAC(6)

//#define idSocketCfg01         0x2380    // idSocketNo(1) szNICCfgName05
//#define idSocketCfg02         0x2381    // idSocketNo(1) szSktAddr(1)
//#define idSocketCfg03         0x2382    // idSocketNo(1) szSktAddr(2)
//#define idSocketCfg04         0x2383    // idSocketNo(1) szSktAddr(3)
//#define idSocketCfg05         0x2384    // idSocketNo(1) szSktAddr(4)
//#define idSocketCfg06         0x2385    // idSocketNo(1) szNICCfgName06
//#define idSocketCfg07         0x2386    // idSocketNo(1) szNICCfgName07
//#define idSocketCfg08         0x2387    // idSocketNo(1) szNICCfgName08
//#define idSocketCfg09         0x2388    // idSocketNo(2) szNICCfgName05
//#define idSocketCfg10         0x2389    // idSocketNo(2) szSktAddr(1)
//#define idSocketCfg11         0x238A    // idSocketNo(2) szSktAddr(2)
//#define idSocketCfg12         0x238B    // idSocketNo(2) szSktAddr(3)
//#define idSocketCfg13         0x238C    // idSocketNo(2) szSktAddr(4)
//#define idSocketCfg14         0x238D    // idSocketNo(2) szNICCfgName06
//#define idSocketCfg15         0x238E    // idSocketNo(2) szNICCfgName07
//#define idSocketCfg16         0x238F    // idSocketNo(2) szNICCfgName08
//#define idSocketCfg17         0x2390    // idSocketNo(3) szNICCfgName05
//#define idSocketCfg18         0x2391    // idSocketNo(3) szSktAddr(1)
//#define idSocketCfg19         0x2392    // idSocketNo(3) szSktAddr(2)
//#define idSocketCfg20         0x2393    // idSocketNo(3) szSktAddr(3)
//#define idSocketCfg21         0x2394    // idSocketNo(3) szSktAddr(4)
//#define idSocketCfg22         0x2395    // idSocketNo(3) szNICCfgName06
//#define idSocketCfg23         0x2396    // idSocketNo(3) szNICCfgName07
//#define idSocketCfg24         0x2397    // idSocketNo(3) szNICCfgName08
//#define idSocketCfg25         0x2398    // idSocketNo(4) szNICCfgName05
//#define idSocketCfg26         0x2399    // idSocketNo(4) szSktAddr(1)
//#define idSocketCfg27         0x239A    // idSocketNo(4) szSktAddr(2)
//#define idSocketCfg28         0x239B    // idSocketNo(4) szSktAddr(3)
//#define idSocketCfg29         0x239C    // idSocketNo(4) szSktAddr(4)
//#define idSocketCfg30         0x239D    // idSocketNo(4) szNICCfgName06
//#define idSocketCfg31         0x239E    // idSocketNo(4) szNICCfgName07
//#define idSocketCfg32         0x239F    // idSocketNo(4) szNICCfgName08
//#define idSocketCfg33         0x23A0    // idSocketNo(5) szNICCfgName05
//#define idSocketCfg34         0x23A1    // idSocketNo(5) szSktAddr(1)
//#define idSocketCfg35         0x23A2    // idSocketNo(5) szSktAddr(2)
//#define idSocketCfg36         0x23A3    // idSocketNo(5) szSktAddr(3)
//#define idSocketCfg37         0x23A4    // idSocketNo(5) szSktAddr(4)
//#define idSocketCfg38         0x23A5    // idSocketNo(5) szNICCfgName06
//#define idSocketCfg39         0x23A6    // idSocketNo(5) szNICCfgName07
//#define idSocketCfg40         0x23A7    // idSocketNo(5) szNICCfgName08
//#define idSocketCfg41         0x23A8    // idSocketNo(6) szNICCfgName05
//#define idSocketCfg42         0x23A9    // idSocketNo(6) szSktAddr(1)
//#define idSocketCfg43         0x23AA    // idSocketNo(6) szSktAddr(2)
//#define idSocketCfg44         0x23AB    // idSocketNo(6) szSktAddr(3)
//#define idSocketCfg45         0x23AC    // idSocketNo(6) szSktAddr(4)
//#define idSocketCfg46         0x23AD    // idSocketNo(6) szNICCfgName06
//#define idSocketCfg47         0x23AE    // idSocketNo(6) szNICCfgName07
//#define idSocketCfg48         0x23AF    // idSocketNo(6) szNICCfgName08

////
//#define idSocketMode01        0x23C0    //
//#define idSocketMode02        0x23C1    //
//#define idSocketMode03        0x23C2    //
//#define idSocketMode04        0x23C3    //

// 串口
#define idUARTCfgName01       0x2490    //
#define idUARTCfgName02       0x2491    //
#define idUARTCfgName03       0x2492    //
#define idUARTCfgName04       0x2493    //
#define idUARTCfgName05       0x2494    //
// 检验
#define idUartParity0         0x2498    //
#define idUartParity1         0x2499    //
#define idUartParity2         0x249A    //
// 速率
#define idUartBaud01          0x24A0    //
#define idUartBaud02          0x24A1    //
#define idUartBaud03          0x24A2    //
#define idUartBaud04          0x24A3    //
#define idUartBaud05          0x24A4    //

#define idUartUnit01          0x24A6    //
#define idUartUnit02          0x24A7    //

#define idCtrlUnit01          0x24AA    //
#define idCtrlUnit02          0x24AB    //
#define idCtrlUnit03          0x24AC    //
#define idCtrlUnit04          0x24AD    //

//#define idUartCfg0             0x24B0    // idUartName(1) szUARTCfgName01
//#define idUartCfg1             0x24B1    // idUartName(1) szUARTCfgName02
//#define idUartCfg2             0x24B2    // idUartName(1) szUARTCfgName03
//#define idUartCfg3             0x24B3    // idUartName(1) szUARTCfgName04
//#define idUartCfg4             0x24B4    // idUartName(1) szUARTCfgName05
//#define idUartCfg5             0x24B5    // idUartName(2) szUARTCfgName01
//#define idUartCfg6             0x24B6    // idUartName(2) szUARTCfgName02
//#define idUartCfg7             0x24B7    // idUartName(2) szUARTCfgName03
//#define idUartCfg8             0x24B8    // idUartName(2) szUARTCfgName04
//#define idUartCfg9             0x24B9    // idUartName(2) szUARTCfgName05

// 以太网配置
#define idEthNetCfg01         0x2500    //
#define idEthNetCfg02         0x2501    //
#define idEthNetCfg03         0x2502    //
#define idEthNetCfg04         0x2503    //
#define idEthNetCfg05         0x2504    //
#define idEthNetCfg06         0x2505    //
#define idEthNetCfg07         0x2506    //
#define idEthNetCfg08         0x2507    //
#define idEthNetCfg09         0x2508    //
#define idEthNetCfg10         0x2509    //
#define idEthNetCfg11         0x250A    //
#define idEthNetCfg12         0x250B    //
#define idEthNetCfg13         0x250C    //
#define idEthNetCfg14         0x250D    //
#define idEthNetCfg15         0x250E    //
#define idEthNetCfg16         0x250F    //

//#define idEthNetCfg21         0x2520    //
//#define idEthNetCfg22         0x2521    //
//#define idEthNetCfg23         0x2522    //
//#define idEthNetCfg24         0x2523    //
//#define idEthNetCfg25         0x2524    //
//#define idEthNetCfg26         0x2525    //
//#define idEthNetCfg27         0x2526    //
//#define idEthNetCfg28         0x2527    //
//#define idEthNetCfg29         0x2528    //
//#define idEthNetCfg30         0x2529    //
//#define idEthNetCfg31         0x252A    //
//#define idEthNetCfg32         0x252B    //
//#define idEthNetCfg33         0x252C    //
//#define idEthNetCfg34         0x252D    //
//#define idEthNetCfg35         0x252E    //
//#define idEthNetCfg36         0x252F    //

//#define idEthNetCfg41         0x2540    //
//#define idEthNetCfg42         0x2541    //
//#define idEthNetCfg43         0x2542    //
//#define idEthNetCfg44         0x2543    //
//#define idEthNetCfg45         0x2544    //
//#define idEthNetCfg46         0x2545    //
//#define idEthNetCfg47         0x2546    //
//#define idEthNetCfg48         0x2547    //
//#define idEthNetCfg49         0x2548    //
//#define idEthNetCfg50         0x2549    //
//#define idEthNetCfg51         0x254A    //
//#define idEthNetCfg52         0x254B    //
//#define idEthNetCfg53         0x254C    //
//#define idEthNetCfg54         0x254D    //
//#define idEthNetCfg55         0x254E    //
//#define idEthNetCfg56         0x254F    //

//#define idEthNetCfg61         0x2560    //
//#define idEthNetCfg62         0x2561    //
//#define idEthNetCfg63         0x2562    //
//#define idEthNetCfg64         0x2563    //
//#define idEthNetCfg65         0x2564    //
//#define idEthNetCfg66         0x2565    //
//#define idEthNetCfg67         0x2566    //
//#define idEthNetCfg68         0x2567    //
//#define idEthNetCfg69         0x2568    //
//#define idEthNetCfg70         0x2569    //
//#define idEthNetCfg71         0x256A    //
//#define idEthNetCfg72         0x256B    //
//#define idEthNetCfg73         0x256C    //
//#define idEthNetCfg74         0x256D    //
//#define idEthNetCfg75         0x256E    //
//#define idEthNetCfg76         0x256F    //

//// GOOSE 配置
//#define idGOOSEPub01           0x2580    //
//#define idGOOSEPub02           0x2581    //
//#define idGOOSEPub03           0x2582    //
//#define idGOOSEPub04           0x2583    //
//#define idGOOSEPub05           0x2584    //
//#define idGOOSEPub06           0x2585    //
//#define idGOOSEPub07           0x2586    //
//#define idGOOSEPub10           0x2590    //
//#define idGOOSEPub11           0x2591    //
//#define idGOOSEPub12           0x2592    //
//#define idGOOSEPub13           0x2593    //
//#define idGOOSEPub14           0x2594    //
//#define idGOOSEPub15           0x2595    //
//#define idGOOSEPub16           0x2596    //
//#define idGOOSEPub17           0x2597    //
//#define idGOOSEPub18           0x2598    //
//#define idGOOSEPub19           0x2599    //
//#define idGOOSEPub20           0x259A    //
//#define idGOOSEPub21           0x259B    //
//#define idGOOSEPub22           0x259C    //
//#define idGOOSEPub23           0x259D    //
//#define idGOOSEPub24           0x259E    //
//#define idGOOSEPub25           0x259F    //

//#define idGOOSESub01           0x25B0    //
//#define idGOOSESub02           0x25B1    //
//#define idGOOSESub03           0x25B2    //
//#define idGOOSESub04           0x25B3    //
//#define idGOOSESub05           0x25B4    //
//#define idGOOSESub06           0x25B5    //
//#define idGOOSESub07           0x25B6    //
//#define idGOOSESub08           0x25B7    //
//#define idGOOSESub09           0x25B8    //
//#define idGOOSESub10          0x25B9    //
//#define idGOOSESub11          0x25BA    //
//#define idGOOSESub12          0x25BB    //
//#define idGOOSESub13          0x25BC    //
//#define idGOOSESub14           0x25BD    //
//#define idGOOSESub15           0x25BE    //
//#define idGOOSESub16           0x25BF    //

// 事件描述
#define idEventName00         0x3000    //
#define idEventName01         0x3001    //
#define idEventName02         0x3002    //
#define idEventName03         0x3003    //
#define idEventName04         0x3004    //
#define idEventName05         0x3005    //
//#define idEventName06         0x3006    //
//#define idEventName07         0x3007    //
#define idEventName08         0x3008    //
#define idEventName09         0x3009    //
#define idEventName10         0x300A    //
#define idEventName11         0x300B    //
#define idEventName12         0x300C    //
#define idEventName13         0x300D    //
#define idEventName14         0x300E    //
#define idEventName15         0x300F    //
#define idEventName16         0x3010    //
#define idEventName17         0x3011    //
#define idEventName18         0x3012    //
#define idEventName19         0x3013    //
#define idEventName20         0x3014    //
#define idEventName21         0x3015    //
//#define idEventName22         0x3016    //
//#define idEventName23         0x3017    //
//#define idEventName24         0x3018    //
//#define idEventName25         0x3019    //
//#define idEventName26         0x301A    //
//#define idEventName27         0x301B    //
//#define idEventName28         0x301C    //
//#define idEventName29         0x301D    //

#define idEventName30         0x3020    //
#define idEventName31         0x3021    //
#define idEventName32         0x3022    //
#define idEventName33         0x3023    //
#define idEventName34         0x3024    //
#define idEventName35         0x3025    //
#define idEventName36         0x3026    //
#define idEventName37         0x3027    //
//#define idEventName38         0x3028    //
//#define idEventName39         0x3029    //

//#define idEventName40         0x3030    //
//#define idEventName41         0x3031    //
//#define idEventName42         0x3032    //
//#define idEventName43         0x3033    //
//#define idEventName44         0x3034    //
//#define idEventName45         0x3035    //
//#define idEventName46         0x3036    //
//#define idEventName47         0x3037    //
//#define idEventName48         0x3038    //
//#define idEventName49         0x3039    //

#define idEventName50         0x3040    //
#define idEventName51         0x3041    //
#define idEventName52         0x3042    //
#define idEventName53         0x3043    //
#define idEventName54         0x3044    //
#define idEventName55         0x3045    //
#define idEventName56         0x3046    //
#define idEventName57         0x3047    //
#define idEventName58         0x3048    //
#define idEventName59         0x3049    //

#define idEventName60         0x3050    //
#define idEventName61         0x3051    //
#define idEventName62         0x3052    //
#define idEventName63         0x3053    //
#define idEventName64         0x3054    //
#define idEventName65         0x3055    //
#define idEventName66         0x3056    //
#define idEventName67         0x3057    //
//#define idEventName68         0x3058    //
//#define idEventName69         0x3059    //

//#define idEventName70         0x3060    //
//#define idEventName71         0x3061    //
//#define idEventName72         0x3062    //
//#define idEventName73         0x3063    //
//#define idEventName74         0x3064    //
//#define idEventName75         0x3065    //
//#define idEventName76         0x3066    //
//#define idEventName77         0x3067    //
//#define idEventName78         0x3068    //
//#define idEventName79         0x3069    //

//#define idEventName80         0x3070    //
//#define idEventName81         0x3071    //
//#define idEventName82         0x3072    //
//#define idEventName83         0x3073    //
//#define idEventName84         0x3074    //
//#define idEventName85         0x3075    //
//#define idEventName86         0x3076    //
//#define idEventName87         0x3077    //
//#define idEventName88         0x3078    //

// 事件动作名称
#define idEventAct00          0x30A0    //

#define idEventAct11          0x30A2    //
#define idEventAct12          0x30A3    //

#define idEventAct21          0x30A5    //
#define idEventAct22          0x30A6    //

#define idEventAct31          0x30A8    //
#define idEventAct32          0x30A9    //

#define idEventCOMBrk         0x30C0    // 通信中断
#define idEventCOMRum         0x30C1    // 通信恢复

// 面板测试
#define idMTHoldKeyExit       0x3100    //

// 定值名称
#define idDZName01            0x4000    //
#define idDZName02            0x4001    //
#define idDZName03            0x4002    //
#define idDZName04            0x4003    //
#define idDZName05            0x4004    //
#define idDZName06            0x4005    //
#define idDZName07            0x4006    //
#define idDZName08            0x4007    //
#define idDZName09            0x4008    //
#define idDZName10            0x4009    //
#define idDZName11            0x400A    //
#define idDZName12            0x400B    //
#define idDZName13            0x400C    //
#define idDZName14            0x400D    //
#define idDZName15            0x400E    //
#define idDZName16            0x400F    //
#define idDZName17            0x4010    //
#define idDZName18            0x4011    //
#define idDZName19            0x4012    //
#define idDZName20            0x4013    //
#define idDZName21            0x4014    //
#define idDZName22            0x4015    //
#define idDZName23            0x4016    //
#define idDZName24            0x4017    //
#define idDZName25            0x4018    //
#define idDZName26            0x4019    //
#define idDZName27            0x401A    //
#define idDZName28            0x401B    //
#define idDZName29            0x401C    //
#define idDZName30            0x401D    //
#define idDZName31            0x401E    //
#define idDZName32            0x401F    //
#define idDZName33            0x4020    //
#define idDZName34            0x4021    //
#define idDZName35            0x4022    //
#define idDZName36            0x4023    //
#define idDZName37            0x4024    //
#define idDZName38            0x4025    //
#define idDZName39            0x4026    //
#define idDZName40            0x4027    //
#define idDZName41            0x4028    //
#define idDZName42            0x4029    //
#define idDZName43            0x402A    //
#define idDZName44            0x402B    //
#define idDZName45            0x402C    //
#define idDZName45            0x402C    //
#define idDZName46            0x402D    //
#define idDZName47            0x402E    //
#define idDZName48            0x402F    //
#define idDZName49            0x4030    //
#define idDZName50            0x4031    //
#define idDZName51            0x4032    //
#define idDZName52            0x4033    //
#define idDZName53            0x4034    //
#define idDZName54            0x4035    //
#define idDZName55            0x4036    //
#define idDZName56            0x4037    //
#define idDZName57            0x4038    //
#define idDZName58            0x4039    //
#define idDZName59            0x403A    //
#define idDZName60            0x403B    //
#define idDZName61            0x403C    //
#define idDZName62            0x403D    //
#define idDZName63            0x403E    //
#define idDZName64            0x403F    //
#define idDZName65            0x4040    //
#define idDZName66            0x4041    //
#define idDZName67            0x4042    //
#define idDZName68            0x4043    //
#define idDZName69            0x4044    //
#define idDZName70            0x4045    //
#define idDZName71            0x4046    //
#define idDZName72            0x4047    //
#define idDZName73            0x4048    //
#define idDZName74            0x4049    //
#define idDZName75            0x404A    //
#define idDZName76            0x404B    //
#define idDZName77            0x404C    //
#define idDZName78            0x404D    //
#define idDZName79            0x404E    //
#define idDZName80            0x404F    //
#define idDZName81            0x4050    //
#define idDZName82            0x4051    //
#define idDZName83            0x4052    //
#define idDZName84            0x4053    //
#define idDZName85            0x4054    //
#define idDZName86            0x4055    //
#define idDZName87            0x4056    //
#define idDZName88            0x4057    //
#define idDZName89            0x4058    //
#define idDZName90            0x4059    //
#define idDZName91            0x405A    //
#define idDZName92            0x405B    //
#define idDZName93            0x405C    //
#define idDZName94            0x405D    //
#define idDZName95            0x405E    //
#define idDZName96            0x405F    //
#define idDZName97            0x4060    //
#define idDZName98            0x4061    //
#define idDZName99            0x4062    //
#define idDZName100           0x4063    //
#define idDZName101           0x4064    //
#define idDZName102           0x4065    //
#define idDZName103           0x4066    //
#define idDZName104           0x4067    //

#define idDZName105           0x4068    //
#define idDZName106           0x4069    //
#define idDZName107           0x406A    //
#define idDZName108           0x406B    //
#define idDZName109           0x406C    //
#define idDZName110           0x406D    //
#define idDZName111           0x406E    //
#define idDZName112           0x406F    //

// 反时限特性
#define idInverse0            0x4100    //
#define idInverse1            0x4101    //
#define idInverse2            0x4102    //
//极性
#define idTA0                 0x4103    //
#define idTA1                 0x4104    //
//重合方式
#define idCHStyle0            0x4105    //
#define idCHStyle1            0x4106    //
#define idCHStyle2            0x4107    //

// 软压板
#define idYBName00            0x4230    //
#define idYBName01            0x4231    //
#define idYBName02            0x4232    //
#define idYBName03            0x4233    //
#define idYBName04            0x4234    //
#define idYBName05            0x4235    //
#define idYBName06            0x4236    //
#define idYBName07            0x4237    //
#define idYBName08            0x4238    //
#define idYBName09            0x4239    //
#define idYBName10            0x423A    //
#define idYBName11            0x423B    //
#define idYBName12            0x423C    //
#define idYBName13            0x423D    //
#define idYBName14            0x423E    //
#define idYBName15            0x423F    //
#define idYBName16            0x4240    //
#define idYBName17            0x4241    //
#define idYBName18            0x4242    //
#define idYBName19            0x4243    //
#define idYBName20            0x4244    //
#define idYBName21            0x4245    //
#define idYBName22            0x4246    //
#define idYBName23            0x4247    //
#define idYBName24            0x4248    //
#define idYBName25            0x4249    //
#define idYBName26            0x424A    //
#define idYBName27            0x424B    //
#define idYBName28            0x424C    //
#define idYBName29            0x424D    //
#define idYBName30            0x424E    //
#define idYBName31            0x424F    //
#define idYBName32            0x4250    //
#define idYBName33            0x4251    //
#define idYBName34            0x4252    //
#define idYBName35            0x4253    //
#define idYBName36            0x4254    //
#define idYBName37            0x4255    //
#define idYBName38            0x4256    //
#define idYBName39            0x4257    //
#define idYBName40            0x4258    //
#define idYBName41            0x4259    //
#define idYBName42            0x425A    //
#define idYBName43            0x425B    //
#define idYBName44            0x425C    //
#define idYBName45            0x425D    //
#define idYBName46            0x425E    //
#define idYBName47            0x425F    //
#define idYBName48            0x4260    //
#define idYBName49            0x4261    //
#define idYBName50            0x4262    //
#define idYBName51            0x4263    //
#define idYBName52            0x4264    //
#define idYBName53            0x4265    //
#define idYBName54            0x4266    //
#define idYBName55            0x4267    //
#define idYBName56            0x4268    //
#define idYBName57            0x4269    //
#define idYBName58            0x426A    //
#define idYBName59            0x426B    //
#define idYBName60            0x426C    //
#define idYBName61            0x426D    //
#define idYBName62            0x426E    //
#define idYBName63            0x426F    //
#define idYBName64            0x4270    //
#define idYBName65            0x4271    //
#define idYBName66            0x4272    //
#define idYBName67            0x4273    //
#define idYBName68            0x4274    //
#define idYBName69            0x4275    //
#define idYBName70            0x4276    //
#define idYBName71            0x4277    //
#define idYBName72            0x4278    //
#define idYBName73            0x4279    //
#define idYBName74            0x427A    //
#define idYBName75            0x427B    //
#define idYBName76            0x427C    //
#define idYBName77            0x427D    //
#define idYBName78            0x427E    //
#define idYBName79            0x427F    //
#define idYBName80            0x4280    //
#define idYBName81            0x4281    //
#define idYBName82            0x4282    //
#define idYBName83            0x4283    //
#define idYBName84            0x4284    //
#define idYBName85            0x4285    //
#define idYBName86            0x4286    //
#define idYBName87            0x4287    //
#define idYBName88            0x4288    //
#define idYBName89            0x4289    //
#define idYBName90            0x428A    //
#define idYBName91            0x428B    //
#define idYBName92            0x428C    //
#define idYBName93            0x428D    //
#define idYBName94            0x428E    //
#define idYBName95            0x428F    //
#define idYBName96            0x4290    //
#define idYBName97            0x4291    //
#define idYBName98            0x4292    //
#define idYBName99            0x4293    //

// 开入
#define idDIName00            0x4500    //  idDIName(1)
#define idDIName01            0x4501    //  idDIName(2)
#define idDIName02            0x4502    //  idDIName(3)
#define idDIName03            0x4503    //  idDIName(4)
#define idDIName04            0x4504    //  idDIName(5)
#define idDIName05            0x4505    //  idDIName(6)
#define idDIName06            0x4506    //  idDIName(7)
#define idDIName07            0x4507    //  idDIName(8)
#define idDIName08            0x4508    //  idDIName(9)
#define idDIName09            0x4509    //  idDIName(10)
#define idDIName10            0x450A    //  idDIName(11)
#define idDIName11            0x450B    //  idDIName(12)
#define idDIName12            0x450C    //  idDIName(13)
#define idDIName13            0x450D    //  idDIName(14)
#define idDIName14            0x450E    //  idDIName(15)

// 开入默认名称
#define idDIAlias00            0x4600    //  idDIAlias(1)
#define idDIAlias01            0x4601    //  idDIAlias(2)
#define idDIAlias02            0x4602    //  idDIAlias(3)
#define idDIAlias03            0x4603    //  idDIAlias(4)
#define idDIAlias04            0x4604    //  idDIAlias(5)
#define idDIAlias05            0x4605    //  idDIAlias(6)
#define idDIAlias06            0x4606    //  idDIAlias(7)
#define idDIAlias07            0x4607    //  idDIAlias(8)
#define idDIAlias08            0x4608    //  idDIAlias(9)
#define idDIAlias09            0x4609    //  idDIAlias(10)
#define idDIAlias10            0x460A    //  idDIAlias(11)
#define idDIAlias11            0x460B    //  idDIAlias(12)
#define idDIAlias12            0x460C    //  idDIAlias(13)
#define idDIAlias13            0x460D    //  idDIAlias(14)
#define idDIAlias14            0x460E    //  idDIAlias(15)
#define idDIAlias15            0x460F    //  idDIAlias(16)
#define idDIAlias16            0x4610    //  idDIAlias(17)
#define idDIAlias17            0x4611    //  idDIAlias(18)
#define idDIAlias18            0x4612    //  idDIAlias(19)
#define idDIAlias19            0x4613    //  idDIAlias(20)
#define idDIAlias20            0x4614    //  idDIAlias(21)
#define idDIAlias21            0x4615    //  idDIAlias(22)
#define idDIAlias22            0x4616    //  idDIAlias(23)
#define idDIAlias23            0x4617    //  idDIAlias(24)
#define idDIAlias24            0x4618    //  idDIAlias(25)
#define idDIAlias25            0x4619    //  idDIAlias(26)
#define idDIAlias26            0x461A    //  idDIAlias(27)
#define idDIAlias27            0x461B    //  idDIAlias(28)
#define idDIAlias28            0x461C    //  idDIAlias(29)
#define idDIAlias29            0x461D    //  idDIAlias(30)
#define idDIAlias30            0x461E    //  idDIAlias(31)
#define idDIAlias31            0x461F    //  idDIAlias(32)
#define idDIAlias32            0x4620    //  idDIAlias(33)
#define idDIAlias33            0x4621    //  idDIAlias(34)
#define idDIAlias34            0x4622    //  idDIAlias(35)
#define idDIAlias35            0x4623    //  idDIAlias(36)
#define idDIAlias36            0x4624    //  idDIAlias(37)
#define idDIAlias37            0x4625    //  idDIAlias(38)
#define idDIAlias38            0x4626    //  idDIAlias(39)
#define idDIAlias39            0x4627    //  idDIAlias(40)
// 开入别名
#define idDIAliasName00       0x4700    //
#define idDIAliasName01       0x4701    //
#define idDIAliasName02       0x4702    //
#define idDIAliasName03       0x4703    //
#define idDIAliasName04       0x4704    //
#define idDIAliasName05       0x4705    //
#define idDIAliasName06       0x4706    //
#define idDIAliasName07       0x4707    //
#define idDIAliasName08       0x4708    //
#define idDIAliasName09       0x4709    //
#define idDIAliasName10       0x470A    //
#define idDIAliasName11       0x470B    //
#define idDIAliasName12       0x470C    //
#define idDIAliasName13       0x470D    //
#define idDIAliasName14       0x470E    //
#define idDIAliasName15       0x470F    //
#define idDIAliasName16       0x4710    //
#define idDIAliasName17       0x4711    //
#define idDIAliasName18       0x4712    //
#define idDIAliasName19       0x4713    //
#define idDIAliasName20       0x4714    //
#define idDIAliasName21       0x4715    //
#define idDIAliasName22       0x4716    //
#define idDIAliasName23       0x4717    //
#define idDIAliasName24       0x4718    //
#define idDIAliasName25       0x4719    //
#define idDIAliasName26       0x471A    //
#define idDIAliasName27       0x471B    //
#define idDIAliasName28       0x471C    //
#define idDIAliasName29       0x471D    //
#define idDIAliasName30       0x471E    //
#define idDIAliasName31       0x471F    //
#define idDIAliasName32       0x4720    //
#define idDIAliasName33       0x4721    //
#define idDIAliasName34       0x4722    //
#define idDIAliasName35       0x4723    //
#define idDIAliasName36       0x4724    //
#define idDIAliasName37       0x4725    //
#define idDIAliasName38       0x4726    //
#define idDIAliasName39       0x4727    //
#define idDIAliasName40       0x4728    //
#define idDIAliasName41       0x4729    //
#define idDIAliasName42       0x472A    //
#define idDIAliasName43       0x472B    //
#define idDIAliasName44       0x472C    //
#define idDIAliasName45       0x472D    //
#define idDIAliasName46       0x472E    //
#define idDIAliasName47       0x472F    //
#define idDIAliasName48       0x4730    //
#define idDIAliasName49       0x4731    //
#define idDIAliasName50       0x4732    //
#define idDIAliasName51       0x4733    //
#define idDIAliasName52       0x4734    //
#define idDIAliasName53       0x4735    //
#define idDIAliasName54       0x4736    //
#define idDIAliasName55       0x4737    //
#define idDIAliasName56       0x4738    //
#define idDIAliasName57       0x4739    //
#define idDIAliasName58       0x473A    //
#define idDIAliasName59       0x473B    //
// 开入传动
#define idDITestName00        0x4800    //  idDITestName(1)
#define idDITestName01        0x4801    //  idDITestName(2)
#define idDITestName02        0x4802    //  idDITestName(3)
#define idDITestName03        0x4803    //  idDITestName(4)
#define idDITestName04        0x4804    //  idDITestName(5)
#define idDITestName05        0x4805    //  idDITestName(6)
#define idDITestName06        0x4806    //  idDITestName(7)
#define idDITestName07        0x4807    //  idDITestName(8)
#define idDITestName08        0x4808    //  idDITestName(9)
#define idDITestName09        0x4809    //  idDITestName(10)
#define idDITestName10        0x480A    //  idDITestName(11)
#define idDITestName11        0x480B    //  idDITestName(12)
#define idDITestName12        0x480C    //  idDITestName(13)
#define idDITestName13        0x480D    //  idDITestName(14)
#define idDITestName14        0x480E    //  idDITestName(15)
#define idDITestName15        0x480F    //  idDITestName(16)
#ifndef DEVTYPE_CK
#define idDITestNameE0        0x4810    //  idDITestName(48)
#define idDITestNameE1        0x4811    //  idDITestName(49)
#define idDITestNameE2        0x4812    //  idDITestName(50)
#define idDITestNameE3        0x4813    //  idDITestName(51)
#else
#define idDITestName16        0x4810    //  idDITestName(17)
#define idDITestName17        0x4811    //  idDITestName(18)
#define idDITestName18        0x4812    //  idDITestName(19)
#define idDITestName19        0x4813    //  idDITestName(20)
#define idDITestName20        0x4814    //  idDITestName(21)
#define idDITestName21        0x4815    //  idDITestName(22)
#define idDITestName22        0x4816    //  idDITestName(23)
#define idDITestName23        0x4817    //  idDITestName(24)
#define idDITestName24        0x4818    //  idDITestName(25)
#define idDITestName25        0x4819    //  idDITestName(26)
#define idDITestName26        0x481A    //  idDITestName(27)
#define idDITestName27        0x481B    //  idDITestName(28)
#define idDITestName28        0x481C    //  idDITestName(29)
#define idDITestName29        0x481D    //  idDITestName(30)
#define idDITestName30        0x481E    //  idDITestName(31)
#define idDITestName31        0x481F    //  idDITestName(32)
#define idDITestName32        0x4820    //  idDITestName(33)
#define idDITestName33        0x4821    //  idDITestName(34)
#define idDITestName34        0x4822    //  idDITestName(35)
#define idDITestName35        0x4823    //  idDITestName(36)
#define idDITestName36        0x4824    //  idDITestName(37)
#define idDITestName37        0x4825    //  idDITestName(38)
#define idDITestName38        0x4826    //  idDITestName(39)
#define idDITestName39        0x4827    //  idDITestName(40)
#define idDITestName40        0x4828    //  idDITestName(41)
#define idDITestName41        0x4829    //  idDITestName(42)
#define idDITestName43        0x482A    //  idDITestName(43)
#endif

//  开出状态
#define idRlyState00          0x4A00    //  szEnRlyState
#define idRlyState01          0x4A01    //  idRlyState(1)
#define idRlyState02          0x4A02    //  idRlyState(2)
#define idRlyState03          0x4A03    //  idRlyState(3)
#define idRlyState04          0x4A04    //  idRlyState(4)
#define idRlyState05          0x4A05    //  idRlyState(5)
#define idRlyState06          0x4A06    //  idRlyState(6)
#define idRlyState07          0x4A07    //  idRlyState(7)
#define idRlyState08          0x4A08    //  idRlyState(8)
#define idRlyState09          0x4A09    //  idRlyState(9)
#define idRlyState10          0x4A0A    //  idRlyState(10)
#define idRlyState11          0x4A0B    //  idRlyState(11)
#define idRlyState12          0x4A0C    //  idRlyState(12)
#define idRlyState13          0x4A0D    //  idRlyState(13)
#define idRlyState14          0x4A0E    //  idRlyState(14)
#define idRlyState15          0x4A0F    //  idRlyState(15)
#define idRlyState16          0x4A10    //  idRlyState(16)
#define idRlyState17          0x4A11    //  idRlyState(17)
#define idRlyState18          0x4A12    //  idRlyState(18)

#define idRlyTest00           0x4C10    //
#define idRlyTest01           0x4C11    //
#define idRlyTest02           0x4C12    //
#define idRlyTest03           0x4C13    //
#define idRlyTest04           0x4C14    //
#define idRlyTest05           0x4C15    //
#define idRlyTest06           0x4C16    //
#define idRlyTest07           0x4C17    //
#define idRlyTest08           0x4C18    //
#define idRlyTest09           0x4C19    //
#define idRlyTest10           0x4C1A    //
#ifndef DEVTYPE_CK
#define idRlyTest11           0x4C1B    //
#define idRlyTest12           0x4C1C    //
#define idRlyTest13           0x4C1D    //
#define idRlyTest14           0x4C1E    //
#define idRlyTest15           0x4C1F    //
#define idRlyTest16           0x4C20    //
#define idRlyTest17           0x4C21    //
#define idRlyTest18           0x4C22    //
#endif

// 软遥信
#define idSGName01            0x5000    //
#define idSGName02            0x5001    //
#define idSGName03            0x5002    //
#define idSGName04            0x5003    //
#define idSGName05            0x5004    //
#define idSGName06            0x5005    //
#define idSGName07            0x5006    //
#define idSGName08            0x5007    //
#define idSGName09            0x5008    //
#define idSGName10            0x5009    //
#define idSGName11            0x500A    //
#define idSGName12            0x500B    //
#define idSGName13            0x500C    //
#define idSGName14            0x500D    //
#define idSGName15            0x500E    //
#define idSGName16            0x500F    //
#define idSGName17            0x5010    //
#define idSGName18            0x5011    //
#define idSGName19            0x5012    //
#define idSGName20            0x5013    //
#define idSGName21            0x5014    //
#define idSGName22            0x5015    //
#define idSGName23            0x5016    //
#define idSGName24            0x5017    //
#define idSGName25            0x5018    //
#define idSGName26            0x5019    //
#define idSGName27            0x501A    //
#define idSGName28            0x501B    //
#define idSGName29            0x501C    //
#define idSGName30            0x501D    //
#define idSGName31            0x501E    //
#define idSGName32            0x501F    //
#define idSGName33            0x5020    //
#define idSGName34            0x5021    //
#define idSGName35            0x5022    //
#define idSGName36            0x5023    //
#define idSGName37            0x5024    //
#define idSGName38            0x5025    //
#define idSGName39            0x5026    //
//#define idSGName40            0x5027    //
//#define idSGName41            0x5028    //
//#define idSGName42            0x5029    //
//#define idSGName43            0x502A    //
//#define idSGName44            0x502B    //
//#define idSGName45            0x502C    //
//#define idSGName46            0x502D    //
//#define idSGName47            0x502E    //
//#define idSGName48            0x502F    //
//#define idSGName49            0x5030    //
//#define idSGName50            0x5031    //
//#define idSGName51            0x5032    //
//#define idSGName52            0x5033    //
//#define idSGName53            0x5034    //
//#define idSGName54            0x5035    //
//#define idSGName55            0x5036    //
//#define idSGName56            0x5037    //
//#define idSGName57            0x5038    //
//#define idSGName58            0x5039    //
//#define idSGName59            0x503A    //
//#define idSGName60            0x503B    //
//#define idSGName61            0x503C    //
//#define idSGName62            0x503D    //
//#define idSGName63            0x503E    //
//#define idSGName64            0x503F    //
//#define idSGName65            0x5040    //
//#define idSGName66            0x5041    //
//#define idSGName67            0x5042    //
//#define idSGName68            0x5043    //
//#define idSGName69            0x5044    //
//#define idSGName70            0x5045    //
//#define idSGName71            0x5046    //
//#define idSGName72            0x5047    //
//#define idSGName73            0x5048    //
//#define idSGName74            0x5049    //
//#define idSGName75            0x504A    //
//#define idSGName76            0x504B    //
//#define idSGName77            0x504C    //
//#define idSGName78            0x504D    //
//#define idSGName79            0x504E    //
//#define idSGName80            0x504F    //
//#define idSGName81            0x5050    //
//#define idSGName82            0x5051    //
//#define idSGName83            0x5052    //
//#define idSGName84            0x5053    //
//#define idSGName85            0x5054    //
//#define idSGName86            0x5055    //
//#define idSGName87            0x5056    //
//#define idSGName88            0x5057    //
//#define idSGName89            0x5058    //
//#define idSGName90            0x5059    //
//#define idSGName91            0x505A    //
//#define idSGName92            0x505B    //
//#define idSGName93            0x505C    //
//#define idSGName94            0x505D    //
//#define idSGName95            0x505E    //
//#define idSGName96            0x505F    //
//#define idSGName97            0x5060    //
//#define idSGName98            0x5061    //
//#define idSGName99            0x5062    //
//#define idSGName100           0x5063    //

// GOOSE事件
#define idETHNETBreak         0x5201   //
#define idGSCOMMBreak         0x5208   //
#define idGSEvtName01         0x5210   //
#define idGSEvtName02         0x5211   //
#define idGSEvtName03         0x5212   //
#define idGSEvtName04         0x5213   //
#define idGSEvtName05         0x5214   //
#define idGSEvtName06         0x5215   //
#define idGSEvtName07         0x5216   //
#define idGSEvtName08         0x5217   //
#define idGSEvtName09         0x5218   //
#define idGSEvtName10         0x5219   //
#define idGSEvtName11         0x521A   //
#define idGSEvtName12         0x521B   //
#define idGSEvtName13         0x521C   //
#define idGSEvtName14         0x521D   //
#define idGSEvtName15         0x521E   //
#define idGSEvtName16         0x521F   //
#define idGSEvtName17         0x5220   //
#define idGSEvtName18         0x5221   //
#define idGSEvtName19         0x5222   //
#define idGSEvtName20         0x5223   //
#define idGSEvtName21         0x5224   //
#define idGSEvtName22         0x5225   //
#define idGSEvtName23         0x5226   //
#define idGSEvtName24         0x5227   //
#define idGSEvtName25         0x5228   //
#define idGSEvtName26         0x5229   //
#define idGSEvtName27         0x522A   //
#define idGSEvtName28         0x522B   //
#define idGSEvtName29         0x522C   //
#define idGSEvtName30         0x522D   //
#define idGSEvtName31         0x522E   //
#define idGSEvtName32         0x522F   //
#define idGSEvtName33         0x5230   //
#define idGSEvtName34         0x5231   //
#define idGSEvtName35         0x5232   //
#define idGSEvtName36         0x5233   //
#define idGSEvtName37         0x5234   //
#define idGSEvtName38         0x5235   //
#define idGSEvtName39         0x5236   //
#define idGSEvtName40         0x5237   //

#if METERING_EN > 0
// 电度计量
#define idTypeMetering0       0x5300
#define idTypeMetering1       0x5301
#define idTypeMetering2       0x5302
#define idLabelMetering00     0x5303   //
#define idLabelMetering01     0x5304   //
#define idLabelMetering02     0x5305   //
#define idLabelMetering03     0x5306   //
#define idLabelMetering10     0x5307   //
#define idLabelMetering11     0x5308   //
#endif

// 谐波分量
#define idHARM_THD            0x6000    //
#define idHARMName01          0x6001    //
#define idHARMName02          0x6002    //
#define idHARMName03          0x6003    //
#define idHARMName04          0x6004    //

#define idHARM01              0x6110    // idHARM01(IA)
#define idHARM02              0x6111    // idHARM02(IA)
#define idHARM03              0x6112    // idHARM03(IA)
#define idHARM04              0x6113    // idHARM04(IA)
#define idHARM05              0x6114    // idHARM01(IA)
#define idHARM06              0x6115    // idHARM02(IA)
#define idHARM07              0x6116    // idHARM03(IA)
#define idHARM08              0x6117    // idHARM04(IA)
#define idHARM09              0x6118    // idHARM01(IB)
#define idHARM10              0x6119    // idHARM02(IB)
#define idHARM11              0x611A    // idHARM03(IB)
#define idHARM12              0x611B    // idHARM04(IB)
#define idHARM13              0x611C    // idHARM01(IB)
#define idHARM14              0x611D    // idHARM02(IB)
#define idHARM15              0x611E    // idHARM03(IB)
#define idHARM16              0x611F    // idHARM04(IB)
#define idHARM17              0x6120    // idHARM01(IC)
#define idHARM18              0x6121    // idHARM02(IC)
#define idHARM19              0x6122    // idHARM03(IC)
#define idHARM20              0x6123    // idHARM04(IC)
#define idHARM21              0x6124    // idHARM01(IC)
#define idHARM22              0x6125    // idHARM02(IC)
#define idHARM23              0x6126    // idHARM03(IC)
#define idHARM24              0x6127    // idHARM04(IC)
#define idHARM25              0x6128    // idHARM01(UA)
#define idHARM26              0x6129    // idHARM02(UA)
#define idHARM27              0x612A    // idHARM03(UA)
#define idHARM28              0x612B    // idHARM04(UA)
#define idHARM29              0x612C    // idHARM01(UA)
#define idHARM30              0x612D    // idHARM02(UA)
#define idHARM31              0x612E    // idHARM03(UA)
#define idHARM32              0x612F    // idHARM04(UA)
#define idHARM33              0x6130    // idHARM01(UB)
#define idHARM34              0x6131    // idHARM02(UB)
#define idHARM35              0x6132    // idHARM03(UB)
#define idHARM36              0x6133    // idHARM04(UB)
#define idHARM37              0x6134    // idHARM01(UB)
#define idHARM38              0x6135    // idHARM02(UB)
#define idHARM39              0x6136    // idHARM03(UB)
#define idHARM40              0x6137    // idHARM04(UB)
#define idHARM41              0x6138    // idHARM01(UC)
#define idHARM42              0x6139    // idHARM02(UC)
#define idHARM43              0x613A    // idHARM03(UC)
#define idHARM44              0x613B    // idHARM04(UC)
#define idHARM45              0x613C    // idHARM01(UC)
#define idHARM46              0x613D    // idHARM02(UC)
#define idHARM47              0x613E    // idHARM03(UC)
#define idHARM48              0x613F    // idHARM04(UC)
#define idHarmCapBase         0x6148    //
#define idHarmCapOrder        0x6149    //
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 获取对应的字符串
// 用中文字符串对应到界面语言对应的字符串
const char* GetMultiLangString(uint32_t uStrId);
//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
#endif
