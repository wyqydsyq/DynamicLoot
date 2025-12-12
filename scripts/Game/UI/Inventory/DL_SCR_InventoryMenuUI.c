modded class SCR_InventoryMenuUI : ChimeraMenuBase
{	
	override protected void Action_CloseInventory()
	{
		array<BaseInventoryStorageComponent> traverseStorage = {};
		if (m_wLootStorage)
		{
			SCR_InventoryStorageBaseUI storageUIHandler = SCR_InventoryStorageBaseUI.Cast( m_wLootStorage.FindHandler( SCR_InventoryStorageBaseUI ) );
			storageUIHandler.GetTraverseStorage(traverseStorage);
		}

		if (!traverseStorage.IsEmpty())
		{
			BaseInventoryStorageComponent storage = traverseStorage[0];
			if (storage)
			{
				IEntity entity = storage.GetOwner();
				m_InventoryManager.PlayItemSound(entity, SCR_SoundEvent.SOUND_CONTAINER_CLOSE);
			}
		}
		else
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_CLOSE);
		}
		
		GetGame().GetInputManager().RemoveActionListener("Inventory_Drag", EActionTrigger.DOWN, Action_DragDown);
		GetGame().GetInputManager().RemoveActionListener("Inventory", EActionTrigger.DOWN, Action_CloseInventory);

		if (m_pVicinity)
		{
			m_pVicinity.SetItemOfInterest(null);
			m_iVicinityDiscoveryRadius = m_pVicinity.GetDiscoveryRadius();
			m_pVicinity.ManipulationComplete();
		}

		MenuManager menuManager = GetGame().GetMenuManager();
		ChimeraMenuPreset menu = ChimeraMenuPreset.Inventory20Menu;

		MenuBase inventoryMenu = menuManager.FindMenuByPreset(menu); // prototype inventory
		if (inventoryMenu)
			menuManager.CloseMenuByPreset(menu);
		
		if  (m_PlayerRenderAttributes)
			m_PlayerRenderAttributes.ResetDeltaRotation();

		if (m_Player)
		{
			m_CharController = SCR_CharacterControllerComponent.Cast(m_Player.GetCharacterController());
			if (m_CharController)
				m_CharController.m_OnLifeStateChanged.Remove(LifeStateChanged);
		}
		
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.GetOnControllableDeleted().Remove(OnControllableDeleted);
		
		if (m_pCharacterWidgetHelper)
			m_pCharacterWidgetHelper.Destroy();
		
		m_pCharacterWidgetHelper = null;

		HideItemInfo();
		HideDamageInfo();
		
		if ( m_pItemInfo )
			m_pItemInfo.Destroy();
		
		if ( m_pDamageInfo )
			m_pDamageInfo.Destroy();
		
		m_pItemInfo = null;
		m_pDamageInfo = null;

		if (m_InventoryManager)
			m_InventoryManager.OnInventoryMenuClosed();
	}
}
