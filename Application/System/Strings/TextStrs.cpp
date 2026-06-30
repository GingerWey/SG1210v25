//-----------------------------------------------------------------------------
/*
  File        : TextStrs.h
  Version     : V1.01
  By          : 银网科技

  Description :按语言类型选择资源文件

  Date       : 2017.12.10
*/
//-----------------------------------------------------------------------------
#include "TextStrs.h"

#include "DevFixed.h"

//#include "GUICfg.h"

// 不同语言的头文件
#include "Strings/StrsCHS.h"
#include "Strings/StrsENG.h"
//=============================================================================
// 本地数据结构
//-----------------------------------------------------------------------------
typedef struct tagMultiLangStrings
{
  uint32_t    Ident;
  const char* Chs;
  const char* Eng;
} TMultiLangStrings;
//=============================================================================
// 本地数据
//-----------------------------------------------------------------------------
constexpr TMultiLangStrings listMultiStrings[] =
{
   { idDevFuncUVTC,        "智能欠压脱扣控制装置",         "Undervoltage Tirp Controller"}
  ,{ idDevCopyright,       "Copyright(c) 2026 SilverGrid",  "Copyright(c) 2026 SilverGrid" }
  ,{ idGPDevFamiry,        "SG1210",                       "SG1210"        }

  
//// 定值区
//#if COLOR_BITS == 1
//  ,{ idLabHBlockNo,        "%u区",                "BN.%u"            }
//#else
//  ,{ idLabHBlockNo,        "定值 %u 区",          "SBLK No.:%u"      }
//#endif
//  ,{ idHintBlockNo,        "当前是：第 %u 区",    "The ACTIVE Block is No: %u" }

//  ,{ idStatusLocal,       "就地",                   "Local"           }
//  ,{ idStatusRemote,      "远方",                   "Remote"          }
//  ,{ idStatusSpCharging,  "充电",                   "Chargeing"       }
//  ,{ idStatusSpCharged,   "贮能",                   "Charged"         }
// 界面语言                                                              
  ,{ idLANG_CHS,          "简体中文/CHS",           "简体中文/CHS"       }
  ,{ idLANG_ENG,          "英文/English",           "英语/English"       }
// 经销商列表
  ,{ idVedor01,            "银网科技",              "SilverGrid"        }
  ,{ idCulture01,          "动力无限",              "Dynamic infinite"  }
// 常用按键
  ,{ idBtnCapOk,          "确定",                   "Ok"                 }
  ,{ idBtnCapCancel,      "放弃",                   "Cancel"             }
  ,{ idBtnCapClose,       "关闭",                   "Close"              }
//  ,{ idBtnCapYes,         "是",                    "Yes"              }
//  ,{ idBtnCapNo,          "否",                    "No"               }
//  ,{ idBtnCapBegin,       "开始",                  "Start"            }
  ,{ idEnableYes,          "允许",                  "Enable"            }
  ,{ idEnableNo,           "禁止",                  "Disable"           }

  ,{ idSysMode0,           "正常工作",              "Normal"            }
  ,{ idSysMode1,           "正在起动",              "Starting"          }
  ,{ idSysMode2,           "正在校准",              "Calibrating"       }
  ,{ idSysMode3,           "传动试验",              "Testing"           }
  ,{ idSysMode4,           "正在维护参数",          "Configuring"      }
  ,{ idSysMode5,           "软件故障",              "FW Fatal"          }
  ,{ idSysMode6,           "硬件故障",              "HW Fatal"          }
  ,{ idSysMode7,           "功能类型错误",          "Unknown FunType"  }
  ,{ idSysMode8,           "定值错误",              "Setting Fatal"     }
  ,{ idSysModeX,           "未知状态",              "Unknown Mode"      }

// 系统出错信息           
  ,{ idDevFault01,        "内部错误",               "System fault!"              }
  ,{ idDevFault02,        "内存不足",               "No Memory!"                 }
  ,{ idDevFault03,        "指针为空",               "Null Pointer!"              }
  ,{ idDevFault04,        "装置类型错误",           "Unknown Device type!"       }
  ,{ idDevFault05,        "不识别的功能类型",       "Unknown Function type!"     }
  ,{ idDevFault06,        "参数错误",               "Illegal Parament!"          }
  ,{ idDevFault07,        "寄存器地址错误",         "Illegal Register address!"  }
  ,{ idDevFault08,        "令牌错误",               "Token error!"               }
  ,{ idDevFault09,        "给定的值不合法！",       "Illegal threshold!"        }
  ,{ idDevFault10,        "硬件非法访问！",         "Illegal hardware access!"  }
//  // 装置故障             
  ,{ iaCriticalInfo,      "捕捉到内部故障",         "Find a error!"         }
//  ,{ iaCriticalFW,        szCriticalFW,               szEngCriticalFW     }
//  ,{ iaCriticalHW,        szCriticalHW,               szEngCriticalHW     }
//  ,{ iaCriticalFT,        szCriticalFT,               szEngCriticalFT     }
//  ,{ iaCriticalDZ,        szCriticalDZ,               szEngCriticalDZ     }
//  ,{ iaCriticalUKN,       szCriticalUKN,              szEngCriticalUKN    }
//                                                                         
//  ,{ iaCriticalFmt,       szCriticalFmt,              szEngCriticalFmt    }
//  ,{ iaCriticalRst,       szCriticalRst,              szEngCriticalRst    }
//  ,{ iaCriticalOk,        szCriticalOk,               szEngCriticalOk     }
//  ,{ iaCriticalFailed,    szCriticalFailed,           szEngCriticalFailed }
// 主窗体
  ,{ idMainLabel01,       "输入：",                  "Vin ="            }
  ,{ idMainLabel02,       "输出：",                  "Vout="    }
  ,{ idMainLabel03,       "充电：",                  "Ichg="           }
  ,{ idMainLabel04,       "放电：",                  "Iexp="           }
  ,{ idMainLabel05,       "温度：",                  "Tbat="           }
  ,{ idMainLabel06,       "电池：",                  "Batt="           }

  ,{ idMainStat01,        "关闭",                   "Shutdown"          }
  ,{ idMainStat02,        "启动",                   "Startup"           }
  ,{ idMainStat03,        "监控",                   "Monitor"           }
  ,{ idMainStat04,        "失电保持",               "Holding"           }
  ,{ idMainStat05,        "得电闭锁",               "Latch"             }
  ,{ idMainStat06,        "旁路运行",               "Passby"            }
  ,{ idMainStat07,        "待关机",                 "Wait Shutdown"     }
  ,{ idMainStat08,        "待机",                   "Shutdown "         }
  ,{ idMainStat09,        "装置故障",               "Device Fatal"      }

  ,{ idMainFmSt01,        "地址：%d",               "DevAddr:%d"        }

  ,{ idMainFmSt02,        "%0.1fV",                 "%0.1fV"           }
  ,{ idMainFmSt03,        "%0.3fA",                 "%0.3fA"          }
  ,{ idMainFmSt04,        "%0.1f℃",                "%0.1f℃"          }
  ,{ idMainFmSt05,        "%0.1f%%",                "%0.1f%%"          }

//  ,{ idMainFmSt51,        "失压",                 "Voltage loss"    }
//  ,{ idMainFmSt52,        "交流电压",             "AC Voltage"       }
  
// 主菜单
  ,{ idMenuCaption,       "SG1210功能菜单",         "SG1210Meun"        }
  
  ,{ idMenuName1,         "测量",                   "Measure"           }
  ,{ idMenuDesp1,         "查询实时测量数据",       "Browse Realtime data" }

  ,{ idMenuName2,         "事件",                   "Event"             }
  ,{ idMenuDesp2,         "调阅事件记录",           "Browse event log"  }

  ,{ idMenuName3,         "日志",                   "Log"               }
  ,{ idMenuDesp3,         "调阅装置运行日志",       "Browse device log" }

  ,{ idMenuName4,         "录波",                   "Wavelog",        }
  ,{ idMenuDesp4,         "调阅事件录波记录",       "Browse Wavelog " }

  ,{ idMenuName5,         "调试",                   "Test",            }
  ,{ idMenuDesp5,         "装置传动试验",           "Device trip test" }

  ,{ idMenuName6,         "逻辑",                   "Logic",       }
  ,{ idMenuDesp6,         "配置装置保护参数",       "Config protection logic" }

  ,{ idMenuName7,         "装置",                   "Device",       }
  ,{ idMenuDesp7,         "配置装置工作参数",       "Config device" }

  ,{ idMenuName8,         "串口",                   "RS485",       }
  ,{ idMenuDesp8,         "配置串口工作参数",       "Config RS485" }

  ,{ idMenuName9,         "网口",                   "Ethernet",       }
  ,{ idMenuDesp9,         "配置以太口工作参数",     "Config ethernet" }

  ,{ idMenuName10,         "关于",                  "About",       }
  ,{ idMenuDesp10,         "查看装置版本信息",      "Device information" }

//// 恢复默认值
//  ,{ idMenuRstSetting,    szMenuRstSetting,              szEngMenuRstSetting     }
//  ,{ idMenuRstSetCfgOk,   szMenuRstSetCfgOk,             szEngMenuRstSetCfgOk    }
//  ,{ idMenuRstSetCfgErr,  szMenuRstSetCfgErr,            szEngMenuRstSetCfgErr   }
//  ,{ idMenuRstDevCfg,     szMenuRstDevCfg,               szEngMenuRstDevCfg      }
//  ,{ idMenuRstDevCfgOk,   szMenuRstDevCfgOk,             szEngMenuRstDevCfgOk    }
//  ,{ idMenuRstDevCfgErr,  szMenuRstDevCfgErr,            szEngMenuRstDevCfgErr   }
//// 菜单内提示信息
//  ,{ idFuncLocked,        szFuncLocked,                  szEngFuncLocked         }
//  ,{ idFuncFailed,        szFuncFailed,                  szEngFuncFailed         }
//  ,{ idFuncUnanth,        szFuncUnanth,                  szEngFuncUnanth         }
// 清除事件记录 确认
  ,{ idClrEventLogCfm,    szClrEventLogCfm,              szEngClrEventLogCfm  }
  ,{ idClrAlarmLogCfm,    szClrAlarmLogCfm,              szEngClrAlarmLogCfm  }
  ,{ idClrFaultLogCfm,    szClrFaultLogCfm,              szEngClrFaultLogCfm  }
  ,{ idClrEventLogOk,     szClrEventLogOk,               szEngClrEventLogOk   }
  ,{ idClrAlarmLogOk,     szClrAlarmLogOk,               szEngClrAlarmLogOk   }
  ,{ idClrFaultLogOk,     szClrFaultLogOk,               szEngClrFaultLogOk   }
#if WAVELOGGER_EN > 0
// 清波形记录
  ,{ idClrWaveLogCfm,     szClrWaveLogCfm,               szEngClrWaveLogCfm   }
  ,{ idClrWaveLogOk,      szClrWaveLogOk,                szEngClrWaveLogOk    }
  ,{ idClrWaveLogFail,    szClrWaveLogFail,              szEngClrWaveLogFail  }
#endif
#if METERING_EN > 0
// 清波电度数据
  ,{ idClrMeteringCfm,     szClrMeteringCfm,             szEngClrMeteringCfm  }
  ,{ idClrMeteringOk,      szClrMeteringOk,              szEngClrMeteringOk   }
  ,{ idClrMeteringFail,    szClrMeteringFail,            szEngClrMeteringFail }
#endif
// 确认装置重启
  ,{ idDeviceResetCfm,    szDeviceResetCfm,              szEngDeviceResetCfm  }
// 用户登录窗体
  ,{ idLoginCap1,         szLoginCap1,                   szEngLoginCap1       }
  ,{ idLoginCap2,         szLoginCap2,                   szEngLoginCap2       }
  ,{ idLoginCap3,         szLoginCap3,                   szEngLoginCap3       }
  ,{ idLoginApply,        "申请码：",                    "Apply code:"        }
  ,{ idLoginChkErrCap,    szLoginChkErrCap,              szEngLoginChkErrCap  }
  ,{ idLoginChkErrTxt,    szLoginChkErrTxt,              szEngLoginChkErrTxt  }
  ,{ idSetPSWDCap1,       szSetPSWDCap1,                 szEngSetPSWDCap1     }
  ,{ idSetPSWDCap2,       szSetPSWDCap2,                 szEngSetPSWDCap2     }
  ,{ idSetPSWDCFM,        szSetPSWDCFM,                  szEngSetPSWDCFM      }
  ,{ idSetPSWDCFM1,       szSetPSWDCFM1,                 szEngSetPSWDCFM1     }
  ,{ idSetPSWDCFM2,       szSetPSWDCFM2,                 szEngSetPSWDCFM2     }
  ,{ idSetPSWDOk,         szSetPSWDOk,                   szEngSetPSWDOk       }
  ,{ idSetPSWDOk1,        szSetPSWDOk1,                  szEngSetPSWDOk1      }
  ,{ idSetPSWDOk2,        szSetPSWDOk2,                  szEngSetPSWDOk2      }
  
// 切换运行区窗体
//#if COLOR_BITS == 1
//  ,{ idSSFmCurBlock,      "%u",                          "%u"                              }
//#else
//  ,{ idSSFmCurBlock,      "当前%u区",                    "Current: %u"                     }
//#endif
//  ,{ idSSFmCaption1,      "选择运行区：",                "Seclect ACTIVE Setting Block:"   }
//  ,{ idSSFmCaption2,      "选择定值区：",                "Seclect Setting Block:"          }
//  ,{ idSSFmCaption3,      "选择定值区：",                "Seclect Setting Block:"          }
//  ,{ idSSFmCaption4,      "复制到目标区：",              "Select destination:"             }
//  ,{ idSSDestIsCur,       szSSDestIsCur,                 szEngSSDestIsCur                 }
//  ,{ idSSSwtAtvHldOk,     szSSSwtAtvHldOk,               szEngSSSwtAtvHldOk               }
//  ,{ idSSSwtAtvHldErr,    szSSSwtAtvHldErr,              szEngSSSwtAtvHldErr              }
//  ,{ idSSSwtEdtHldErr,    szSSSwtEdtHldErr,              szEngSSSwtEdtHldErr              }
//  ,{ idSSSwtCpyHldOk,     szSSSwtCpyHldOk,               szEngSSSwtCpyHldOk               }
//  ,{ idSSSwtCpyHldErr,    szSSSwtCpyHldErr,              szEngSSSwtCpyHldErr              }
//// 恢复默认定值
//  ,{ idSetModifiedCap,    "保存设置",                    "Save config"                     }
//  ,{ idSetModifiedLbl,    "设置已被修改，\n是否保存？",  "Settings modified,\nSAVE or NOT?" }
//  ,{ idSetSavedOk,        "设置已成功保存。",            "Settings saved."                 }
// 配置日期时间
  ,{ idSetDateTimeCap,    "设置日期时间",                "Set date & time"                   }
//  ,{ idSetClockOk,        "设置时钟成功！",              "Clock modified!"                 }
//// 遥动分合闸
//  ,{ idManuCtrlOp0,       "执行:\n【合闸】操作,\n是否继续?",      "Breaker\nwill be CLOSE\nAre sure?"        }
//  ,{ idManuCtrlOp1,       "执行:\n【分闸】操作,\n是否继续?",      "Breaker\nwill be OPEN.\nAre sure?"        }
//  ,{ idManuCtrlInf0a,     "断路器为:\n【分】状态\n可以【合闸】.", "Breaker\nis OPEN.\nCan CLOSE."            }
//  ,{ idManuCtrlInf0b,     "断路器为:\n【合】状态\n可以【分闸】.", "Breaker\nis CLOSE.\nCan OPEN."            }
//  ,{ idManuCtrlInf1,      "控制回路断线,\n操作不能继续!",         "Control_loop\nwas broken.\nCan't operate" }
//  ,{ idManuCtrlInf2,      "",                                     ""                                       }
//  ,{ idManuCtrlInf3,      "【远方】状态,\n操作不能继续!",         "Remote state.\nCan't operate"             }
// 校准提示
//  ,{ idACCalibHint,       szACCalibHint,               szEngACCalibHint     }
//  ,{ idDCCalibHint0,      szDCCalibHint0,              szEngDCCalibHint0  }
//  ,{ idDCCalibHint1,      szDCCalibHint1,              szEngDCCalibHint1  }
//  ,{ idCalibCap,          szCalibCap,                  szEngCalibCap        }
//  ,{ idCalibOk,           szCalibOk,                   szEngCalibOk         }
//  ,{ idCalibSomeOk,       szCalibSomeOk,               szEngCalibSomeOk     }
//  ,{ idCalibFailed,       szCalibFailed,               szEngCalibFailed     }
//  ,{ idMeaAdjFailed,      szMeaAdjFailed,              szEngMeaAdjFailed    }
// 枚举提示
  ,{ idEnumChanged,       szEnumChanged,               szEngEnumChanged     }
  ,{ idEnumChgRst,        szEnumChgRst,                szEngEnumChgRst      }

// 事件列表窗体
  ,{ idEventCatalog1,     "日志记录",                      "Log record"     }
  ,{ idEventCatalog2,     "报警记录",                      "Alarm log"      }
  ,{ idEventCatalog3,     "事件记录",                      "Fatal log"      }
  ,{ idDateTimeFormat,    "%02u-%02u %02u:%02u\"%02u'%03u", "%02u-%02u %02u:%02u\"%02u'%03u" }
  ,{ idEventEmpty,        "无记录",                      "No Record"                         }
  ,{ idEvtDataEmpty,      "无数据",                      "No Data"                           }

  
// 设备参数
  ,{ idDevCfgName01,      "功能类型：",                "Device Type"         }
  ,{ idDevCfgName02,      "界面语言：",                "Language"            }
  ,{ idDevCfgName03,      "操作口令：",                "Operator Password"   }
  ,{ idDevCfgName04,      "动态口令：",                "Dynamic Password"    }
  ,{ idDevCfgName05,      "超级口令：",                "Supper Password"     }
    
  ,{ idDevCfgName06,      "装置地址：",                "Device Address"      }
  ,{ idDevCfgName07,      "自动控制：",                "Control Enable"      }
  ,{ idDevCfgName08,      "自动关机：",                "Shutdown Enable"     }
  ,{ idDevCfgName09,      "自动合闸：",                "Breaker On"          }
  ,{ idDevCfgName10,      "旁路模式：",                "Passby Enable"       }
  ,{ idDevCfgName11,      "失压门限：",                "Voltage Lowlimit"    }
  ,{ idDevCfgName12,      "额定电压：",                "Output Voltage"      }
  ,{ idDevCfgName13,      "得电延时：",                "Power-on delay"      }
  ,{ idDevCfgName14,      "失电延时：",                "Power-down delay"    }
  ,{ idDevCfgName15,      "关机延时：",                "Shutdown delay"      }
  ,{ idDevCfgName16,      "继电器启动延时：",          "Relay delay"         }
  ,{ idDevCfgName17,      "继电器持续时间：",          "Relay Active timer"  }

// 串口 
  ,{ idUARTCfgName01,     "串行口配置",                "COM1 Config"         }
  ,{ idUARTCfgName02,     "装置地址：",                "Slave Address"       }
  ,{ idUARTCfgName03,     "通信速率：",                "Baudrate"            }
  ,{ idUARTCfgName04,     "校验方式：",                "Parity"              }
  
  ,{ idUartParity0,       "无校验",                    "None"                }
  ,{ idUartParity1,       "偶校验",                    "Even"                }
  ,{ idUartParity2,       "奇校验",                    "Odd"                 }

  ,{ idUartBaud01,        "4800",                      "4800"               }
  ,{ idUartBaud02,        "9600",                      "9600"               }
  ,{ idUartBaud03,        "14400",                     "14400"              }
  ,{ idUartBaud04,        "19200",                     "19200"              }
  ,{ idUartBaud05,        "115200",                    "115200"             }

  ,{ idUartUnit01,        "bps",                       "bps"                }
  ,{ idUartUnit02,        "位",                        "bits"               }

  ,{ idCtrlUnit01,        "%",                         "%"                  }
  ,{ idCtrlUnit02,        "V",                         "V"                  }
  ,{ idCtrlUnit03,        "秒",                        "Sec"                }
  ,{ idCtrlUnit04,        "分",                        "Min"                }

//  ,{ idUartCfg0,     szUartName(1) szUARTCfgName01, szEngUartName(1) szEngUARTCfgName01 }
//  ,{ idUartCfg1,     szUartName(1) szUARTCfgName02, szEngUartName(1) szEngUARTCfgName02 }
//  ,{ idUartCfg2,     szUartName(1) szUARTCfgName03, szEngUartName(1) szEngUARTCfgName03 }
//  ,{ idUartCfg3,     szUartName(1) szUARTCfgName04, szEngUartName(1) szEngUARTCfgName04 }
//  ,{ idUartCfg4,     szUartName(1) szUARTCfgName05, szEngUartName(1) szEngUARTCfgName05 }
//  ,{ idUartCfg5,     szUartName(2) szUARTCfgName01, szEngUartName(2) szEngUARTCfgName01 }
//  ,{ idUartCfg6,     szUartName(2) szUARTCfgName02, szEngUartName(2) szEngUARTCfgName02 }
//  ,{ idUartCfg7,     szUartName(2) szUARTCfgName03, szEngUartName(2) szEngUARTCfgName03 }
//  ,{ idUartCfg8,     szUartName(2) szUARTCfgName04, szEngUartName(2) szEngUARTCfgName04 }
//  ,{ idUartCfg9,     szUartName(2) szUARTCfgName05, szEngUartName(2) szEngUARTCfgName05 }
// 事件描述
  ,{ idEventName00,       szEventName00,           szEngEventName00         }
  ,{ idEventName01,       szEventName01,           szEngEventName01         }
  ,{ idEventName02,       szEventName02,           szEngEventName02         }
  ,{ idEventName03,       szEventName03,           szEngEventName03         }
  ,{ idEventName04,       szEventName04,           szEngEventName04         }
  ,{ idEventName05,       szEventName05,           szEngEventName05         }
//  ,{ idEventName06,       szEventName06,           szEngEventName06         }
//  ,{ idEventName07,       "",                      ""                       }
  ,{ idEventName08,       "看门狗启动",           "WDG active"               }
  ,{ idEventName09,       "主晶振失效",           "HSE fault"                }
  ,{ idEventName10,       "辅晶振失效",           "HSI fault"                }
  ,{ idEventName11,       "RCC失效",              "RCC fault"                }
  ,{ idEventName12,       "时钟电池失效",         "RTC Battary fault"        }
  ,{ idEventName13,       "实时时钟失效",         "RTC fault"                }
  ,{ idEventName14,       "定值存器错误",         "EPROM fault"              }
  ,{ idEventName15,       "录波存器失效",         "ExFlash fault"            }
  ,{ idEventName16,       "AD芯片失效",           "AD fault"                 }
  ,{ idEventName17,       "充电测量失效",         "Charge monitor fault"     }
  ,{ idEventName18,       "放电测量失效",         "Discharge monitor fault"  }
  ,{ idEventName19,       "外部RTC失效",          "ExRTC fault"              }
  ,{ idEventName20,       "装置电池电压低",       "Device Battary low"       }
  ,{ idEventName21,       "调压器故障",           "Rheostat fault"           }
  
  ,{ idEventName30,       "设备参数错误"    ,     "Device config fault"      }
  ,{ idEventName31,       "设备参数初始化"  ,     "Dev-config initialize"    }
  ,{ idEventName32,       "压板和定值错误"  ,     "Set points fault"         }
  ,{ idEventName33,       "压板和定值初始化",     "Set points initialize"    }
  ,{ idEventName34,       "报告记录错误"    ,     "Logs fault"               }
  ,{ idEventName35,       "报告记录初始化"  ,     "Logs initialize"          }
  ,{ idEventName36,       "保护压板定值失效",     "Set points error"         }
  ,{ idEventName37,       "定值自动修复"    ,     "Set points recovered"     }
  ,{ idEventName50,       "修改日期时间"    ,     "Set date_time"            }
  ,{ idEventName51,       "清除【事件记录】"  ,   "Delete [Event logs]"      }
  ,{ idEventName52,       "清除【告警记录】"  ,   "Delete [Alarm logs]"      }
  ,{ idEventName53,       "清除【事故记录】"  ,   "Delete [Fault logs]"      }
  ,{ idEventName54,       "清除【波形记录】"  ,   "Delete [Fault logs]"      }
  ,{ idEventName56,       "恢复默认定值"    ,     "Load default setpoints"   }
  ,{ idEventName57,       "恢复设备参数"    ,     "Load default config"      }
  ,{ idEventName58,       "切换运行定值区"  ,     "swithed ACTIVE block"     }
  ,{ idEventName59,       "复制定值区"      ,     "Copy set points"          }
  ,{ idEventName60,       "修改装置参数"    ,     "Set DEVICE config"        }
  ,{ idEventName61,       "修改压板"        ,     "Set relay plates"       }
  ,{ idEventName62,       "修改定值"        ,     "Set settings"           }
  ,{ idEventName63,       "修改通信参数"    ,     "Set Comm. Config"       }
  ,{ idEventName64,       "修改出口参数"    ,     "Set Relay Outputs"      }
  ,{ idEventName65,       "修改操作口令"    ,     "Set password"             }
  ,{ idEventName66,       "修改超级口令"    ,     "Set password2"            }
  ,{ idEventName67,       "修改模拟量校准"  ,     "Execute Calibration"      }
//  ,{ idEventName48,       "修改装置功能类型",     "Change Device Type"       }
//  ,{ idEventName49,       "修改经销商名称"  ,     "Change Vendor"          }
//  ,{ idEventName50,       "故障信号复归"  ,       "Signal return"          }
//  ,{ idEventName51,       "手动触发录波"  ,       "Wavelog trigger"          }
// 事件动作名称
  ,{ idEventAct00,        "",                     ""                        }

  ,{ idEventAct11,        "合",                   "ON"                      }
  ,{ idEventAct12,        "分",                   "OFF"                     }

  ,{ idEventAct21,        "动作",                 "Active"                  }
  ,{ idEventAct22,        "复归",                 "Return"                  }

  ,{ idEventAct31,        "投入",                 "Active"                  }
  ,{ idEventAct32,        "退出",                 "Return"                  }

//  ,{ idEventCOMBrk,       "中断",                 "Break off"             }
//  ,{ idEventCOMRum,       "恢复",                 "Resumed"               }

//// 面板测试
//  ,{ idMTHoldKeyExit,     "长按【ESC】键退出",    "Hold Key [ESC] to exit." }

//// 定值名称
// 开入名
  ,{ idDIName00,          "开机保持",            "MCU power"                }
  ,{ idDIName01,          "充电器",              "AC power supply"          }
  ,{ idDIName02,          "逆变器工作",          "Invertor Output"          }
  ,{ idDIName03,          "逆变器使能",          "Invertor Enable"          }
  ,{ idDIName04,          "合闸出口",            "Breaker On Signal"        }
  ,{ idDIName05,          "旁路工作",            "Passby Supply"            }
  ,{ idDIName06,          "加热器",              "Heater"                   }
  ,{ idDIName07,          "散热风扇",            "Fan"                      }

//  开出状态
  ,{ idRlyState00,      "合闸出口",              "Breaker-ON Relay"         }
  ,{ idRlyState01,      "旁路开关",              "Passby mode"              }
  ,{ idRlyState02,      "加热器启动",            "Heater"                   }
  ,{ idRlyState03,      "风扇启动",              "Fan"                      }
// 软遥信
  ,{ idSGName01,        "得电闭锁",              "Voltage resume latching"  }
  ,{ idSGName02,        "失压保持",              "Undervoltage sustaining"  }
  ,{ idSGName03,        "自动合闸",              "Breaker turn ON"          }
};
#define NUM_listMultiStrings          NUM_Elements(listMultiStrings)
//=============================================================================
// 全局方法
//-----------------------------------------------------------------------------
// 获取对应的字符串
// 用中文字符串对应到界面语言对应的字符串
const char* GetMultiLangString(uint32_t uStrId)
{

  if( listMultiStrings[0].Ident > uStrId ||
      listMultiStrings[NUM_listMultiStrings - 1].Ident < uStrId )
    return nullptr;

  const TMultiLangStrings* pItem = nullptr;
  uint32_t uBeg = 0, uEnd = NUM_listMultiStrings - 1,
           uPos = (uBeg + uEnd) >> 1;
  while( uEnd - uBeg > 2)
    {
    if( listMultiStrings[uPos].Ident > uStrId )
      uEnd = uPos;
    else if( listMultiStrings[uPos].Ident < uStrId )
      uBeg = uPos;
    else
      {
      pItem = &listMultiStrings[uPos];
      break;
      }

    uPos = (uBeg + uEnd) >> 1;
    }

  if( nullptr == pItem )
    {
    if(listMultiStrings[uPos].Ident == uStrId )
      pItem = &listMultiStrings[uPos];
    else if(listMultiStrings[uBeg].Ident == uStrId )
      pItem = &listMultiStrings[uBeg];
    else if(listMultiStrings[uEnd].Ident == uStrId )
      pItem = &listMultiStrings[uEnd];
    }

  if( nullptr == pItem )
    return nullptr;

  const char* pcString;
  if( 0 == DevConfig.Language )
    pcString = pItem->Chs;
  else
    pcString = pItem->Eng;

  return pcString;
}
//-----------------------------------------------------------------------------

