[EntityEditorProps(category: "GameScripted/Triggers", description: "Categorizes spawn labels")]
class DL_LootContainerTriggerClass: SCR_BaseTriggerEntityClass
{
}

class DL_LootContainerTrigger : SCR_BaseTriggerEntity
{
	DL_LootSystem lootSystem;
	IEntity parent;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if (!Replication.IsServer())
			return;
		
		lootSystem = DL_LootSystem.GetInstance();
		if (!lootSystem)
			return;
		
		parent = owner.GetParent();
		if (parent)
			m_OnActivate.Insert(CreateLootContainer);
	}
	
	override bool ScriptedEntityFilterForQuery(IEntity ent) {
		if (!EntityUtils.IsPlayer(ent))
			return false;
		
		return true;
	};
	
	void CreateLootContainer(IEntity ent)
	{
		lootSystem.CreateLootContainer(parent);
		m_OnActivate.Remove(CreateLootContainer);
		delete this;
	}
}