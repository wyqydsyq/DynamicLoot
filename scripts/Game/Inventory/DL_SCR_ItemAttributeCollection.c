[BaseContainerProps()]
modded class SCR_ItemAttributeCollection: ItemAttributeCollection
{
	bool SetIsVisible(bool visible = false)
	{
		m_bVisible = visible;
		return visible;
	}
};
