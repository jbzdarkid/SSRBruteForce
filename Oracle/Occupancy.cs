using System;

// Token: 0x020000AD RID: 173
public struct Occupancy
{
	// Token: 0x06000AE3 RID: 2787 RVA: 0x000378AD File Offset: 0x00035CAD
	public Occupancy(Coord _pos, Direction _dir, bool _entering, Fraction _position, int _speed)
	{
		this.pos = _pos;
		this.dir = _dir;
		this.entering = _entering;
		this.position = _position;
		this.speed = _speed;
	}

	// Token: 0x06000AE4 RID: 2788 RVA: 0x000378D4 File Offset: 0x00035CD4
	public bool Overlaps(Occupancy other)
	{
		return !this.CompatibleWith(other);
	}

	// Token: 0x06000AE5 RID: 2789 RVA: 0x000378E0 File Offset: 0x00035CE0
	public bool CompatibleWith(Occupancy other)
	{
		if (this.pos != other.pos)
		{
			return true;
		}
		if (this.Static() || other.Static())
		{
			return false;
		}
		if (this.dir != other.dir)
		{
			return false;
		}
		if (this.entering == other.entering)
		{
			return false;
		}
		Occupancy occupancy = ((!this.entering) ? other : this);
		Occupancy occupancy2 = (this.entering ? other : this);
		return !(occupancy.position > occupancy2.position) && occupancy2.TimeTillLeave() <= occupancy.TimeTillLeave();
	}

	// Token: 0x06000AE6 RID: 2790 RVA: 0x000379A4 File Offset: 0x00035DA4
	private Fraction TimeTillLeave()
	{
		return (1 - this.position) / this.speed;
	}

	// Token: 0x06000AE7 RID: 2791 RVA: 0x000379BD File Offset: 0x00035DBD
	public bool Static()
	{
		return this.speed == 0;
	}

	// Token: 0x06000AE8 RID: 2792 RVA: 0x000379C8 File Offset: 0x00035DC8
	public bool Rotating()
	{
		return this.dir == Direction.None && this.speed > 0;
	}

	// Token: 0x040006CE RID: 1742
	public Coord pos;

	// Token: 0x040006CF RID: 1743
	public Direction dir;

	// Token: 0x040006D0 RID: 1744
	public bool entering;

	// Token: 0x040006D1 RID: 1745
	public Fraction position;

	// Token: 0x040006D2 RID: 1746
	public int speed;
}
