//===-- MICmdCmdBreak.cpp ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Overview:    CMICmdCmdBreakInsert            implementation.
//              CMICmdCmdBreakDelete            implementation.
//              CMICmdCmdBreakDisable           implementation.
//              CMICmdCmdBreakEnable            implementation.
//              CMICmdCmdBreakAfter             implementation.
//              CMICmdCmdBreakCondition         implementation.

// Third Party Headers:
#include "lldb/API/SBBreakpointLocation.h"

// In-house headers:
#include "MICmdCmdBreak.h"
#include "MICmnMIResultRecord.h"
#include "MICmnMIValueConst.h"
#include "MICmnMIOutOfBandRecord.h"
#include "MICmnLLDBDebugger.h"
#include "MICmnLLDBDebugSessionInfo.h"
#include "MICmdArgValFile.h"
#include "MICmdArgValNumber.h"
#include "MICmdArgValString.h"
#include "MICmdArgValThreadGrp.h"
#include "MICmdArgValOptionLong.h"
#include "MICmdArgValOptionShort.h"
#include "MICmdArgValListOfN.h"
#include "MICmnStreamStdout.h"

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakInsert constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakInsert::CMICmdCmdBreakInsert(void)
    : m_bBrkPtIsTemp(false)
    , m_bBrkPtIsPending(false)
    , m_nBrkPtIgnoreCount(0)
    , m_bBrkPtEnabled(false)
    , m_bBrkPtCondition(false)
    , m_bBrkPtThreadId(false)
    , m_nBrkPtThreadId(0)
    , m_constStrArgNamedTempBrkPt("t")
    , m_constStrArgNamedHWBrkPt("h")
    , m_constStrArgNamedPendinfBrkPt("f")
    , m_constStrArgNamedDisableBrkPt("d")
    , m_constStrArgNamedTracePt("a")
    , m_constStrArgNamedConditionalBrkPt("c")
    , m_constStrArgNamedInoreCnt("i")
    , m_constStrArgNamedRestrictBrkPtToThreadId("p")
    , m_constStrArgNamedLocation("location")
    , m_constStrArgNamedThreadGroup("thread-group")
{
    // Command factory matches this name with that received from the stdin stream
    m_strMiCmd = "break-insert";

    // Required by the CMICmdFactory when registering *this command
    m_pSelfCreatorFn = &CMICmdCmdBreakInsert::CreateSelf;
}

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakInsert destructor.
// Type:    Overrideable.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakInsert::~CMICmdCmdBreakInsert(void)
{
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The parses the command line options
//          arguments to extract values for each of those arguments.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakInsert::ParseArgs(void)
{
    m_setCmdArgs.Add(*(new CMICmdArgValOptionShort(m_constStrArgNamedTempBrkPt, false, true)));
    // Not implemented m_setCmdArgs.Add( *(new CMICmdArgValOptionShort( m_constStrArgNamedHWBrkPt, false, false ) ) );
    m_setCmdArgs.Add(*(new CMICmdArgValOptionShort(m_constStrArgNamedPendinfBrkPt, false, true,
                                                   CMICmdArgValListBase::eArgValType_StringQuotedNumberPath, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValOptionShort(m_constStrArgNamedDisableBrkPt, false, false)));
    // Not implemented m_setCmdArgs.Add( *(new CMICmdArgValOptionShort( m_constStrArgNamedTracePt, false, false ) ) );
    m_setCmdArgs.Add(*(new CMICmdArgValOptionShort(m_constStrArgNamedConditionalBrkPt, false, true,
                                                   CMICmdArgValListBase::eArgValType_StringQuoted, 1)));
    m_setCmdArgs.Add(
        *(new CMICmdArgValOptionShort(m_constStrArgNamedInoreCnt, false, true, CMICmdArgValListBase::eArgValType_Number, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValOptionShort(m_constStrArgNamedRestrictBrkPtToThreadId, false, true,
                                                   CMICmdArgValListBase::eArgValType_Number, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValString(m_constStrArgNamedLocation, false, true)));
    m_setCmdArgs.Add(
        *(new CMICmdArgValOptionLong(m_constStrArgNamedThreadGroup, false, true, CMICmdArgValListBase::eArgValType_ThreadGrp, 1)));
    return ParseValidateCmdOptions();
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command does work in this function.
//          The command is likely to communicate with the LLDB SBDebugger in here.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakInsert::Execute(void)
{
    CMICMDBASE_GETOPTION(pArgTempBrkPt, OptionShort, m_constStrArgNamedTempBrkPt);
    CMICMDBASE_GETOPTION(pArgThreadGroup, OptionLong, m_constStrArgNamedThreadGroup);
    CMICMDBASE_GETOPTION(pArgLocation, String, m_constStrArgNamedLocation);
    CMICMDBASE_GETOPTION(pArgIgnoreCnt, OptionShort, m_constStrArgNamedInoreCnt);
    CMICMDBASE_GETOPTION(pArgPendingBrkPt, OptionShort, m_constStrArgNamedPendinfBrkPt);
    CMICMDBASE_GETOPTION(pArgDisableBrkPt, OptionShort, m_constStrArgNamedDisableBrkPt);
    CMICMDBASE_GETOPTION(pArgConditionalBrkPt, OptionShort, m_constStrArgNamedConditionalBrkPt);
    CMICMDBASE_GETOPTION(pArgRestrictBrkPtToThreadId, OptionShort, m_constStrArgNamedRestrictBrkPtToThreadId);

    m_bBrkPtEnabled = !pArgDisableBrkPt->GetFound();
    m_bBrkPtIsTemp = pArgTempBrkPt->GetFound();
    m_bHaveArgOptionThreadGrp = pArgThreadGroup->GetFound();
    if (m_bHaveArgOptionThreadGrp)
    {
        MIuint nThreadGrp = 0;
        pArgThreadGroup->GetExpectedOption<CMICmdArgValThreadGrp, MIuint>(nThreadGrp);
        m_strArgOptionThreadGrp = CMIUtilString::Format("i%d", nThreadGrp);
    }
    m_bBrkPtIsPending = pArgPendingBrkPt->GetFound();
    if (pArgLocation->GetFound())
        m_brkName = pArgLocation->GetValue();
    else if (m_bBrkPtIsPending)
    {
        pArgPendingBrkPt->GetExpectedOption<CMICmdArgValString, CMIUtilString>(m_brkName);
    }
    if (pArgIgnoreCnt->GetFound())
    {
        pArgIgnoreCnt->GetExpectedOption<CMICmdArgValNumber, MIuint>(m_nBrkPtIgnoreCount);
    }
    m_bBrkPtCondition = pArgConditionalBrkPt->GetFound();
    if (m_bBrkPtCondition)
    {
        pArgConditionalBrkPt->GetExpectedOption<CMICmdArgValString, CMIUtilString>(m_brkPtCondition);
    }
    m_bBrkPtThreadId = pArgRestrictBrkPtToThreadId->GetFound();
    if (m_bBrkPtCondition)
    {
        pArgRestrictBrkPtToThreadId->GetExpectedOption<CMICmdArgValNumber, MIuint>(m_nBrkPtThreadId);
    }

    // Determine if break on a file line or at a function
    BreakPoint_e eBrkPtType = eBreakPoint_NotDefineYet;
    const CMIUtilString cColon = ":";
    CMIUtilString fileName;
    MIuint nFileLine = 0;
    CMIUtilString strFileFn;
    CMIUtilString rStrLineOrFn;
    // Full path in windows can have : after drive letter. So look for the
    // last colon
    const size_t nPosColon = m_brkName.find_last_of(cColon);
    if (nPosColon != std::string::npos)
    {
        // extract file name and line number from it
        fileName = m_brkName.substr(0, nPosColon);
        rStrLineOrFn = m_brkName.substr(nPosColon + 1, m_brkName.size() - nPosColon - 1);

        if (rStrLineOrFn.empty())
            eBrkPtType = eBreakPoint_ByName;
        else
        {
            MIint64 nValue = 0;
            if (rStrLineOrFn.ExtractNumber(nValue))
            {
                nFileLine = static_cast<MIuint>(nValue);
                eBrkPtType = eBreakPoint_ByFileLine;
            }
            else
            {
                strFileFn = rStrLineOrFn;
                eBrkPtType = eBreakPoint_ByFileFn;
            }
        }
    }

    // Determine if break defined as an address
    lldb::addr_t nAddress = 0;
    if (eBrkPtType == eBreakPoint_NotDefineYet)
    {
        MIint64 nValue = 0;
        if (m_brkName.ExtractNumber(nValue))
        {
            nAddress = static_cast<lldb::addr_t>(nValue);
            eBrkPtType = eBreakPoint_ByAddress;
        }
    }

    // Break defined as an function
    if (eBrkPtType == eBreakPoint_NotDefineYet)
    {
        eBrkPtType = eBreakPoint_ByName;
    }

    // Ask LLDB to create a breakpoint
    bool bOk = MIstatus::success;
    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    lldb::SBTarget sbTarget = rSessionInfo.GetTarget();
    switch (eBrkPtType)
    {
        case eBreakPoint_ByAddress:
            m_brkPt = sbTarget.BreakpointCreateByAddress(nAddress);
            break;
        case eBreakPoint_ByFileFn:
            m_brkPt = sbTarget.BreakpointCreateByName(strFileFn.c_str(), fileName.c_str());
            break;
        case eBreakPoint_ByFileLine:
            m_brkPt = sbTarget.BreakpointCreateByLocation(fileName.c_str(), nFileLine);
            break;
        case eBreakPoint_ByName:
            m_brkPt = sbTarget.BreakpointCreateByName(m_brkName.c_str(), nullptr);
            break;
        case eBreakPoint_count:
        case eBreakPoint_NotDefineYet:
        case eBreakPoint_Invalid:
            bOk = MIstatus::failure;
            break;
    }

    if (bOk)
    {
        if (!m_bBrkPtIsPending && (m_brkPt.GetNumLocations() == 0))
        {
            sbTarget.BreakpointDelete(m_brkPt.GetID());
            SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_LOCATION_NOT_FOUND), m_cmdData.strMiCmd.c_str(), m_brkName.c_str()));
            return MIstatus::failure;
        }

        m_brkPt.SetEnabled(m_bBrkPtEnabled);
        m_brkPt.SetIgnoreCount(m_nBrkPtIgnoreCount);
        if (m_bBrkPtCondition)
            m_brkPt.SetCondition(m_brkPtCondition.c_str());
        if (m_bBrkPtThreadId)
            m_brkPt.SetThreadID(m_nBrkPtThreadId);
    }

    // CODETAG_LLDB_BREAKPOINT_CREATION
    // This is in the main thread
    // Record break point information to be by LLDB event handler function
    CMICmnLLDBDebugSessionInfo::SBrkPtInfo sBrkPtInfo;
    if (!rSessionInfo.GetBrkPtInfo(m_brkPt, sBrkPtInfo))
        return MIstatus::failure;
    sBrkPtInfo.m_id = m_brkPt.GetID();
    sBrkPtInfo.m_bDisp = m_bBrkPtIsTemp;
    sBrkPtInfo.m_bEnabled = m_bBrkPtEnabled;
    sBrkPtInfo.m_bHaveArgOptionThreadGrp = m_bHaveArgOptionThreadGrp;
    sBrkPtInfo.m_strOptThrdGrp = m_strArgOptionThreadGrp;
    sBrkPtInfo.m_nTimes = m_brkPt.GetHitCount();
    sBrkPtInfo.m_strOrigLoc = m_brkName;
    sBrkPtInfo.m_nIgnore = m_nBrkPtIgnoreCount;
    sBrkPtInfo.m_bPending = m_bBrkPtIsPending;
    sBrkPtInfo.m_bCondition = m_bBrkPtCondition;
    sBrkPtInfo.m_strCondition = m_brkPtCondition;
    sBrkPtInfo.m_bBrkPtThreadId = m_bBrkPtThreadId;
    sBrkPtInfo.m_nBrkPtThreadId = m_nBrkPtThreadId;

    bOk = bOk && rSessionInfo.RecordBrkPtInfo(m_brkPt.GetID(), sBrkPtInfo);
    if (!bOk)
    {
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), m_cmdData.strMiCmd.c_str(), m_brkName.c_str()));
        return MIstatus::failure;
    }

    // CODETAG_LLDB_BRKPT_ID_MAX
    if (m_brkPt.GetID() > (lldb::break_id_t)rSessionInfo.m_nBrkPointCntMax)
    {
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_CNT_EXCEEDED), m_cmdData.strMiCmd.c_str(), rSessionInfo.m_nBrkPointCntMax,
                                       m_brkName.c_str()));
        return MIstatus::failure;
    }

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command prepares a MI Record Result
//          for the work carried out in the Execute().
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakInsert::Acknowledge(void)
{
    // Get breakpoint information
    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    CMICmnLLDBDebugSessionInfo::SBrkPtInfo sBrkPtInfo;
    if (!rSessionInfo.RecordBrkPtInfoGet(m_brkPt.GetID(), sBrkPtInfo))
        return MIstatus::failure;

    // MI print
    // "^done,bkpt={number=\"%d\",type=\"breakpoint\",disp=\"%s\",enabled=\"%c\",addr=\"0x%016" PRIx64 "\",func=\"%s\",file=\"%s\",fullname=\"%s/%s\",line=\"%d\",thread-groups=[\"%s\"],times=\"%d\",original-location=\"%s\"}"
    CMICmnMIValueTuple miValueTuple;
    if (!rSessionInfo.MIResponseFormBrkPtInfo(sBrkPtInfo, miValueTuple))
        return MIstatus::failure;

    const CMICmnMIValueResult miValueResultD("bkpt", miValueTuple);
    const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done, miValueResultD);
    m_miResultRecord = miRecordResult;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Required by the CMICmdFactory when registering *this command. The factory
//          calls this function to create an instance of *this command.
// Type:    Static method.
// Args:    None.
// Return:  CMICmdBase * - Pointer to a new command.
// Throws:  None.
//--
CMICmdBase *
CMICmdCmdBreakInsert::CreateSelf(void)
{
    return new CMICmdCmdBreakInsert();
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakDelete constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakDelete::CMICmdCmdBreakDelete(void)
    : m_constStrArgNamedBrkPt("breakpoint")
    , m_constStrArgNamedThreadGrp("thread-group")
{
    // Command factory matches this name with that received from the stdin stream
    m_strMiCmd = "break-delete";

    // Required by the CMICmdFactory when registering *this command
    m_pSelfCreatorFn = &CMICmdCmdBreakDelete::CreateSelf;
}

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakDelete destructor.
// Type:    Overrideable.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakDelete::~CMICmdCmdBreakDelete(void)
{
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The parses the command line options
//          arguments to extract values for each of those arguments.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakDelete::ParseArgs(void)
{
    m_setCmdArgs.Add(
        *(new CMICmdArgValOptionLong(m_constStrArgNamedThreadGrp, false, false, CMICmdArgValListBase::eArgValType_ThreadGrp, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValListOfN(m_constStrArgNamedBrkPt, true, true, CMICmdArgValListBase::eArgValType_Number)));
    return ParseValidateCmdOptions();
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command does work in this function.
//          The command is likely to communicate with the LLDB SBDebugger in here.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakDelete::Execute(void)
{
    CMICMDBASE_GETOPTION(pArgBrkPt, ListOfN, m_constStrArgNamedBrkPt);

    // ATM we only handle one break point ID
    MIuint64 nBrk = UINT64_MAX;
    if (!pArgBrkPt->GetExpectedOption<CMICmdArgValNumber, MIuint64>(nBrk))
    {
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), m_cmdData.strMiCmd.c_str(), m_constStrArgNamedBrkPt.c_str()));
        return MIstatus::failure;
    }

    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    const bool bBrkPt = rSessionInfo.GetTarget().BreakpointDelete(static_cast<lldb::break_id_t>(nBrk));
    if (!bBrkPt)
    {
        const CMIUtilString strBrkNum(CMIUtilString::Format("%d", nBrk));
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), m_cmdData.strMiCmd.c_str(), strBrkNum.c_str()));
        return MIstatus::failure;
    }

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command prepares a MI Record Result
//          for the work carried out in the Execute().
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakDelete::Acknowledge(void)
{
    const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done);
    m_miResultRecord = miRecordResult;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Required by the CMICmdFactory when registering *this command. The factory
//          calls this function to create an instance of *this command.
// Type:    Static method.
// Args:    None.
// Return:  CMICmdBase * - Pointer to a new command.
// Throws:  None.
//--
CMICmdBase *
CMICmdCmdBreakDelete::CreateSelf(void)
{
    return new CMICmdCmdBreakDelete();
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakDisable constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakDisable::CMICmdCmdBreakDisable(void)
    : m_constStrArgNamedThreadGrp("thread-group")
    , m_constStrArgNamedBrkPt("breakpoint")
    , m_bBrkPtDisabledOk(false)
    , m_nBrkPtId(0)
{
    // Command factory matches this name with that received from the stdin stream
    m_strMiCmd = "break-disable";

    // Required by the CMICmdFactory when registering *this command
    m_pSelfCreatorFn = &CMICmdCmdBreakDisable::CreateSelf;
}

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakDisable destructor.
// Type:    Overrideable.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakDisable::~CMICmdCmdBreakDisable(void)
{
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The parses the command line options
//          arguments to extract values for each of those arguments.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakDisable::ParseArgs(void)
{
    m_setCmdArgs.Add(
        *(new CMICmdArgValOptionLong(m_constStrArgNamedThreadGrp, false, false, CMICmdArgValListBase::eArgValType_ThreadGrp, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValListOfN(m_constStrArgNamedBrkPt, true, true, CMICmdArgValListBase::eArgValType_Number)));
    return ParseValidateCmdOptions();
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command does work in this function.
//          The command is likely to communicate with the LLDB SBDebugger in here.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakDisable::Execute(void)
{
    CMICMDBASE_GETOPTION(pArgBrkPt, ListOfN, m_constStrArgNamedBrkPt);

    // ATM we only handle one break point ID
    MIuint64 nBrk = UINT64_MAX;
    if (!pArgBrkPt->GetExpectedOption<CMICmdArgValNumber, MIuint64>(nBrk))
    {
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), m_cmdData.strMiCmd.c_str(), m_constStrArgNamedBrkPt.c_str()));
        return MIstatus::failure;
    }

    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    lldb::SBBreakpoint brkPt = rSessionInfo.GetTarget().FindBreakpointByID(static_cast<lldb::break_id_t>(nBrk));
    if (brkPt.IsValid())
    {
        m_bBrkPtDisabledOk = true;
        brkPt.SetEnabled(false);
        m_nBrkPtId = nBrk;
    }

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command prepares a MI Record Result
//          for the work carried out in the Execute().
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakDisable::Acknowledge(void)
{
    if (m_bBrkPtDisabledOk)
    {
        const CMICmnMIValueConst miValueConst(CMIUtilString::Format("%d", m_nBrkPtId));
        const CMICmnMIValueResult miValueResult("number", miValueConst);
        CMICmnMIValueTuple miValueTuple(miValueResult);
        const CMICmnMIValueConst miValueConst2("n");
        const CMICmnMIValueResult miValueResult2("enabled", miValueConst2);
        miValueTuple.Add(miValueResult2);
        const CMICmnMIValueResult miValueResult3("bkpt", miValueTuple);
        const CMICmnMIOutOfBandRecord miOutOfBandRecord(CMICmnMIOutOfBandRecord::eOutOfBand_BreakPointModified, miValueResult3);
        bool bOk = CMICmnStreamStdout::TextToStdout(miOutOfBandRecord.GetString());

        const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done);
        m_miResultRecord = miRecordResult;
        return bOk;
    }

    const CMIUtilString strBrkPtId(CMIUtilString::Format("%d", m_nBrkPtId));
    const CMICmnMIValueConst miValueConst(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), strBrkPtId.c_str()));
    const CMICmnMIValueResult miValueResult("msg", miValueConst);
    const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Error, miValueResult);
    m_miResultRecord = miRecordResult;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Required by the CMICmdFactory when registering *this command. The factory
//          calls this function to create an instance of *this command.
// Type:    Static method.
// Args:    None.
// Return:  CMICmdBase * - Pointer to a new command.
// Throws:  None.
//--
CMICmdBase *
CMICmdCmdBreakDisable::CreateSelf(void)
{
    return new CMICmdCmdBreakDisable();
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakEnable constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakEnable::CMICmdCmdBreakEnable(void)
    : m_constStrArgNamedThreadGrp("thread-group")
    , m_constStrArgNamedBrkPt("breakpoint")
    , m_bBrkPtEnabledOk(false)
    , m_nBrkPtId(0)
{
    // Command factory matches this name with that received from the stdin stream
    m_strMiCmd = "break-enable";

    // Required by the CMICmdFactory when registering *this command
    m_pSelfCreatorFn = &CMICmdCmdBreakEnable::CreateSelf;
}

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakEnable destructor.
// Type:    Overrideable.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakEnable::~CMICmdCmdBreakEnable(void)
{
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The parses the command line options
//          arguments to extract values for each of those arguments.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakEnable::ParseArgs(void)
{
    m_setCmdArgs.Add(
        *(new CMICmdArgValOptionLong(m_constStrArgNamedThreadGrp, false, false, CMICmdArgValListBase::eArgValType_ThreadGrp, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValListOfN(m_constStrArgNamedBrkPt, true, true, CMICmdArgValListBase::eArgValType_Number)));
    return ParseValidateCmdOptions();
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command does work in this function.
//          The command is likely to communicate with the LLDB SBDebugger in here.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakEnable::Execute(void)
{
    CMICMDBASE_GETOPTION(pArgBrkPt, ListOfN, m_constStrArgNamedBrkPt);

    // ATM we only handle one break point ID
    MIuint64 nBrk = UINT64_MAX;
    if (!pArgBrkPt->GetExpectedOption<CMICmdArgValNumber, MIuint64>(nBrk))
    {
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), m_cmdData.strMiCmd.c_str(), m_constStrArgNamedBrkPt.c_str()));
        return MIstatus::failure;
    }

    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    lldb::SBBreakpoint brkPt = rSessionInfo.GetTarget().FindBreakpointByID(static_cast<lldb::break_id_t>(nBrk));
    if (brkPt.IsValid())
    {
        m_bBrkPtEnabledOk = true;
        brkPt.SetEnabled(false);
        m_nBrkPtId = nBrk;
    }

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command prepares a MI Record Result
//          for the work carried out in the Execute().
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakEnable::Acknowledge(void)
{
    if (m_bBrkPtEnabledOk)
    {
        const CMICmnMIValueConst miValueConst(CMIUtilString::Format("%d", m_nBrkPtId));
        const CMICmnMIValueResult miValueResult("number", miValueConst);
        CMICmnMIValueTuple miValueTuple(miValueResult);
        const CMICmnMIValueConst miValueConst2("y");
        const CMICmnMIValueResult miValueResult2("enabled", miValueConst2);
        miValueTuple.Add(miValueResult2);
        const CMICmnMIValueResult miValueResult3("bkpt", miValueTuple);
        const CMICmnMIOutOfBandRecord miOutOfBandRecord(CMICmnMIOutOfBandRecord::eOutOfBand_BreakPointModified, miValueResult3);
        bool bOk = CMICmnStreamStdout::TextToStdout(miOutOfBandRecord.GetString());

        const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done);
        m_miResultRecord = miRecordResult;
        return bOk;
    }

    const CMIUtilString strBrkPtId(CMIUtilString::Format("%d", m_nBrkPtId));
    const CMICmnMIValueConst miValueConst(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), strBrkPtId.c_str()));
    const CMICmnMIValueResult miValueResult("msg", miValueConst);
    const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Error, miValueResult);
    m_miResultRecord = miRecordResult;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Required by the CMICmdFactory when registering *this command. The factory
//          calls this function to create an instance of *this command.
// Type:    Static method.
// Args:    None.
// Return:  CMICmdBase * - Pointer to a new command.
// Throws:  None.
//--
CMICmdBase *
CMICmdCmdBreakEnable::CreateSelf(void)
{
    return new CMICmdCmdBreakEnable();
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakAfter constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakAfter::CMICmdCmdBreakAfter(void)
    : m_constStrArgNamedThreadGrp("thread-group")
    , m_constStrArgNamedNumber("number")
    , m_constStrArgNamedCount("count")
    , m_nBrkPtId(0)
    , m_nBrkPtCount(0)
{
    // Command factory matches this name with that received from the stdin stream
    m_strMiCmd = "break-after";

    // Required by the CMICmdFactory when registering *this command
    m_pSelfCreatorFn = &CMICmdCmdBreakAfter::CreateSelf;
}

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakAfter destructor.
// Type:    Overrideable.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakAfter::~CMICmdCmdBreakAfter(void)
{
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The parses the command line options
//          arguments to extract values for each of those arguments.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakAfter::ParseArgs(void)
{
    m_setCmdArgs.Add(
        *(new CMICmdArgValOptionLong(m_constStrArgNamedThreadGrp, false, false, CMICmdArgValListBase::eArgValType_ThreadGrp, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValNumber(m_constStrArgNamedNumber, true, true)));
    m_setCmdArgs.Add(*(new CMICmdArgValNumber(m_constStrArgNamedCount, true, true)));
    return ParseValidateCmdOptions();
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command does work in this function.
//          The command is likely to communicate with the LLDB SBDebugger in here.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakAfter::Execute(void)
{
    CMICMDBASE_GETOPTION(pArgNumber, Number, m_constStrArgNamedNumber);
    CMICMDBASE_GETOPTION(pArgCount, Number, m_constStrArgNamedCount);

    m_nBrkPtId = pArgNumber->GetValue();
    m_nBrkPtCount = pArgCount->GetValue();

    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    lldb::SBBreakpoint brkPt = rSessionInfo.GetTarget().FindBreakpointByID(static_cast<lldb::break_id_t>(m_nBrkPtId));
    if (brkPt.IsValid())
    {
        brkPt.SetIgnoreCount(m_nBrkPtCount);

        CMICmnLLDBDebugSessionInfo::SBrkPtInfo sBrkPtInfo;
        if (!rSessionInfo.RecordBrkPtInfoGet(m_nBrkPtId, sBrkPtInfo))
        {
            SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INFO_OBJ_NOT_FOUND), m_cmdData.strMiCmd.c_str(), m_nBrkPtId));
            return MIstatus::failure;
        }
        sBrkPtInfo.m_nIgnore = m_nBrkPtCount;
        rSessionInfo.RecordBrkPtInfo(m_nBrkPtId, sBrkPtInfo);
    }
    else
    {
        const CMIUtilString strBrkPtId(CMIUtilString::Format("%d", m_nBrkPtId));
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), m_cmdData.strMiCmd.c_str(), strBrkPtId.c_str()));
        return MIstatus::failure;
    }

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command prepares a MI Record Result
//          for the work carried out in the Execute().
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakAfter::Acknowledge(void)
{
    const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done);
    m_miResultRecord = miRecordResult;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Required by the CMICmdFactory when registering *this command. The factory
//          calls this function to create an instance of *this command.
// Type:    Static method.
// Args:    None.
// Return:  CMICmdBase * - Pointer to a new command.
// Throws:  None.
//--
CMICmdBase *
CMICmdCmdBreakAfter::CreateSelf(void)
{
    return new CMICmdCmdBreakAfter();
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakCondition constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakCondition::CMICmdCmdBreakCondition(void)
    : m_constStrArgNamedThreadGrp("thread-group")
    , m_constStrArgNamedNumber("number")
    , m_constStrArgNamedExpr("expr")
    , m_constStrArgNamedExprNoQuotes(
          "expression not surround by quotes") // Not specified in MI spec, we need to handle expressions not surrounded by quotes
    , m_nBrkPtId(0)
{
    // Command factory matches this name with that received from the stdin stream
    m_strMiCmd = "break-condition";

    // Required by the CMICmdFactory when registering *this command
    m_pSelfCreatorFn = &CMICmdCmdBreakCondition::CreateSelf;
}

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdBreakCondition destructor.
// Type:    Overrideable.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdBreakCondition::~CMICmdCmdBreakCondition(void)
{
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The parses the command line options
//          arguments to extract values for each of those arguments.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakCondition::ParseArgs(void)
{
    m_setCmdArgs.Add(
        *(new CMICmdArgValOptionLong(m_constStrArgNamedThreadGrp, false, false, CMICmdArgValListBase::eArgValType_ThreadGrp, 1)));
    m_setCmdArgs.Add(*(new CMICmdArgValNumber(m_constStrArgNamedNumber, true, true)));
    m_setCmdArgs.Add(*(new CMICmdArgValString(m_constStrArgNamedExpr, true, true, true, true)));
    m_setCmdArgs.Add(*(new CMICmdArgValListOfN(m_constStrArgNamedExprNoQuotes, true, false,
                                               CMICmdArgValListBase::eArgValType_StringQuotedNumber)));
    return ParseValidateCmdOptions();
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command does work in this function.
//          The command is likely to communicate with the LLDB SBDebugger in here.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakCondition::Execute(void)
{
    CMICMDBASE_GETOPTION(pArgNumber, Number, m_constStrArgNamedNumber);
    CMICMDBASE_GETOPTION(pArgExpr, String, m_constStrArgNamedExpr);

    m_nBrkPtId = pArgNumber->GetValue();
    m_strBrkPtExpr = pArgExpr->GetValue();
    m_strBrkPtExpr += GetRestOfExpressionNotSurroundedInQuotes();

    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    lldb::SBBreakpoint brkPt = rSessionInfo.GetTarget().FindBreakpointByID(static_cast<lldb::break_id_t>(m_nBrkPtId));
    if (brkPt.IsValid())
    {
        brkPt.SetCondition(m_strBrkPtExpr.c_str());

        CMICmnLLDBDebugSessionInfo::SBrkPtInfo sBrkPtInfo;
        if (!rSessionInfo.RecordBrkPtInfoGet(m_nBrkPtId, sBrkPtInfo))
        {
            SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INFO_OBJ_NOT_FOUND), m_cmdData.strMiCmd.c_str(), m_nBrkPtId));
            return MIstatus::failure;
        }
        sBrkPtInfo.m_strCondition = m_strBrkPtExpr;
        rSessionInfo.RecordBrkPtInfo(m_nBrkPtId, sBrkPtInfo);
    }
    else
    {
        const CMIUtilString strBrkPtId(CMIUtilString::Format("%d", m_nBrkPtId));
        SetError(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_BRKPT_INVALID), m_cmdData.strMiCmd.c_str(), strBrkPtId.c_str()));
        return MIstatus::failure;
    }

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command prepares a MI Record Result
//          for the work carried out in the Execute().
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool
CMICmdCmdBreakCondition::Acknowledge(void)
{
    const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done);
    m_miResultRecord = miRecordResult;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Required by the CMICmdFactory when registering *this command. The factory
//          calls this function to create an instance of *this command.
// Type:    Static method.
// Args:    None.
// Return:  CMICmdBase * - Pointer to a new command.
// Throws:  None.
//--
CMICmdBase *
CMICmdCmdBreakCondition::CreateSelf(void)
{
    return new CMICmdCmdBreakCondition();
}

//++ ------------------------------------------------------------------------------------
// Details: A breakpoint expression can be passed to *this command as:
//              a single string i.e. '2' -> ok.
//              a quoted string i.e. "a > 100" -> ok
//              a non quoted string i.e. 'a > 100' -> not ok
//          CMICmdArgValString only extracts the first space separated string, the "a".
//          This function using the optional argument type CMICmdArgValListOfN collects
//          the rest of the expression so that is may be added to the 'a' part to form a
//          complete expression string i.e. "a > 100".
//          If the expression value was guaranteed to be surrounded by quotes them this
//          function would not be necessary.
// Type:    Method.
// Args:    None.
// Return:  CMIUtilString - Rest of the breakpoint expression.
// Throws:  None.
//--
CMIUtilString
CMICmdCmdBreakCondition::GetRestOfExpressionNotSurroundedInQuotes(void)
{
    CMIUtilString strExpression;

    CMICmdArgValListOfN *pArgExprNoQuotes = CMICmdBase::GetOption<CMICmdArgValListOfN>(m_constStrArgNamedExprNoQuotes);
    if (pArgExprNoQuotes != nullptr)
    {
        CMIUtilString strExpression;
        const CMICmdArgValListBase::VecArgObjPtr_t &rVecExprParts(pArgExprNoQuotes->GetExpectedOptions());
        if (!rVecExprParts.empty())
        {
            CMICmdArgValListBase::VecArgObjPtr_t::const_iterator it = rVecExprParts.begin();
            while (it != rVecExprParts.end())
            {
                const CMICmdArgValString *pPartExpr = static_cast<CMICmdArgValString *>(*it);
                const CMIUtilString &rPartExpr = pPartExpr->GetValue();
                strExpression += " ";
                strExpression += rPartExpr;

                // Next
                ++it;
            }
            strExpression = strExpression.Trim();
        }
    }

    return strExpression;
}
