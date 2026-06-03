#include "scriptPCH.h"
#include "stormwrought_ruins.h"

struct instance_stormwrought_ruins : public ScriptedInstance
{
    explicit instance_stormwrought_ruins(Map* pMap) : ScriptedInstance(pMap)
    {
        Initialize();
    }

    uint64 m_uiIghalforGUID;
    uint64 m_uiNetherAltarGUID;
    uint64 m_uiSecretDoorGUID;
    bool m_abSecretDoorSwitches[4];

    void Initialize() override
    {
        m_uiIghalforGUID = 0;
        m_uiNetherAltarGUID = 0;
        m_uiSecretDoorGUID = 0;

        for (bool& bSwitch : m_abSecretDoorSwitches)
            bSwitch = false;
    }

    void OnCreatureCreate(Creature* pCreature) override
    {
        if (pCreature->GetEntry() == NPC_IGHALFOR)
            m_uiIghalforGUID = pCreature->GetGUID();
    }

    void OnObjectCreate(GameObject* pGo) override
    {
        switch (pGo->GetDBTableGUIDLow())
        {
            case GO_SECRET_DOOR_GUID:
                m_uiSecretDoorGUID = pGo->GetGUID();
                break;
            case GO_NETHER_ALTAR_GUID:
                m_uiNetherAltarGUID = pGo->GetGUID();
                break;
        }
    }

    void SetData(uint32 uiType, uint32 uiData) override
    {
        if (uiType != DATA_SECRET_DOOR_ACTIVATED)
            return;

        switch (uiData)
        {
            case GO_SECRET_SWITCH_1_GUID:
                m_abSecretDoorSwitches[0] = true;
                break;
            case GO_SECRET_SWITCH_2_GUID:
                m_abSecretDoorSwitches[1] = true;
                break;
            case GO_SECRET_SWITCH_3_GUID:
                m_abSecretDoorSwitches[2] = true;
                break;
            case GO_SECRET_SWITCH_4_GUID:
                m_abSecretDoorSwitches[3] = true;
                break;
        }

        if (m_abSecretDoorSwitches[0] && m_abSecretDoorSwitches[1] && m_abSecretDoorSwitches[2] && m_abSecretDoorSwitches[3])
        {
            if (GameObject* pDoor = instance->GetGameObject(m_uiSecretDoorGUID))
                if (pDoor->GetGoState() != GO_STATE_ACTIVE)
                    pDoor->UseDoorOrButton(0, true);
        }
    }

    uint32 GetData(uint32 uiType) override
    {
        if (uiType == DATA_SECRET_DOOR_ACTIVATED)
            return m_abSecretDoorSwitches[0] && m_abSecretDoorSwitches[1] && m_abSecretDoorSwitches[2] && m_abSecretDoorSwitches[3];

        return 0;
    }

    uint64 GetData64(uint32 uiType) override
    {
        switch (uiType)
        {
            case DATA_IGHALFOR_GUID:
                return m_uiIghalforGUID;
            case DATA_NETHER_ALTAR_GUID:
                return m_uiNetherAltarGUID;
        }

        return 0;
    }
};

InstanceData* GetInstanceData_instance_stormwrought_ruins(Map* pMap)
{
    return new instance_stormwrought_ruins(pMap);
}

bool GOHello_go_stormwrought_secret_door(Player* /*pPlayer*/, GameObject* pGo)
{
    ScriptedInstance* pInstance = static_cast<ScriptedInstance*>(pGo->GetInstanceData());
    if (!pInstance)
        return true;

    switch (pGo->GetDBTableGUIDLow())
    {
        case GO_SECRET_SWITCH_1_GUID:
        case GO_SECRET_SWITCH_2_GUID:
        case GO_SECRET_SWITCH_3_GUID:
        case GO_SECRET_SWITCH_4_GUID:
            if (pGo->GetGoState() != GO_STATE_ACTIVE)
                pGo->UseDoorOrButton(0, true);
            pInstance->SetData(DATA_SECRET_DOOR_ACTIVATED, pGo->GetDBTableGUIDLow());
            return true;
        case GO_SECRET_DOOR_GUID:
            return true;
    }

    return false;
}

void AddSC_instance_stormwrought_ruins()
{
    Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "instance_stormwrought_ruins";
    pNewscript->GetInstanceData = &GetInstanceData_instance_stormwrought_ruins;
    pNewscript->RegisterSelf();

    pNewscript = new Script;
    pNewscript->Name = "go_stormwrought_secret_door";
    pNewscript->pGOHello = &GOHello_go_stormwrought_secret_door;
    pNewscript->RegisterSelf();
}
