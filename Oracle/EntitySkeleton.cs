using System;

// Token: 0x0200009E RID: 158
public class EntitySkeleton
{
	// Token: 0x060009AB RID: 2475 RVA: 0x00025C54 File Offset: 0x00024054
	public EntitySkeleton()
	{
	}

	// Token: 0x060009AC RID: 2476 RVA: 0x00025C5C File Offset: 0x0002405C
	public EntitySkeleton(Entity other)
	{
		this.pos = other.pos;
		this.direction = other.direction;
		this.stuckto = other.stuckto;
		this.rot = other.rot;
		this.cookdata = other.cookdata;
		this.turndir = other.turndir;
		this.pivot = other.pivot;
		this.dat = other.dat;
		this.tileset = other.tileset;
		this.id = other.id;
		this.type = other.type;
		this.tilenum = other.tilenum;
	}

	// Token: 0x060009AD RID: 2477 RVA: 0x00025D00 File Offset: 0x00024100
	public void CopyFrom(Entity other)
	{
		this.pos = other.pos;
		this.type = other.type;
		this.id = other.id;
		this.tilenum = other.tilenum;
		this.tileset = other.tileset;
		this.direction = other.direction;
		this.dat = other.dat;
		this.stuckto = other.stuckto;
		this.rot = other.rot;
		this.cookdata = other.cookdata;
		this.turndir = other.turndir;
		this.pivot = other.pivot;
	}

	// Token: 0x060009AE RID: 2478 RVA: 0x00025DA0 File Offset: 0x000241A0
	public void CopyFrom(EntitySkeleton other)
	{
		this.pos = other.pos;
		this.type = other.type;
		this.id = other.id;
		this.tilenum = other.tilenum;
		this.tileset = other.tileset;
		this.direction = other.direction;
		this.dat = other.dat;
		this.stuckto = other.stuckto;
		this.rot = other.rot;
		this.cookdata = other.cookdata;
		this.turndir = other.turndir;
		this.pivot = other.pivot;
	}

	// Token: 0x040005A0 RID: 1440
	public Coord pos;

	// Token: 0x040005A1 RID: 1441
	public EntType type;

	// Token: 0x040005A2 RID: 1442
	public int id;

	// Token: 0x040005A3 RID: 1443
	public int tilenum;

	// Token: 0x040005A4 RID: 1444
	public int tileset;

	// Token: 0x040005A5 RID: 1445
	public Direction direction;

	// Token: 0x040005A6 RID: 1446
	public string dat;

	// Token: 0x040005A7 RID: 1447
	public int stuckto;

	// Token: 0x040005A8 RID: 1448
	public int rot;

	// Token: 0x040005A9 RID: 1449
	public int cookdata;

	// Token: 0x040005AA RID: 1450
	public Direction turndir;

	// Token: 0x040005AB RID: 1451
	public int pivot;
}
