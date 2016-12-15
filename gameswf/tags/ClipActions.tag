{
	u16					reserved
	ClipEventFlags		allEventFlags
	ClipActionRecord[]	clipActionRecords

	if version <= 5 {
		u16				clipActionEndFlag
	}
	if version >= 6 {
		u32				clipActionEndFlag
	}
}