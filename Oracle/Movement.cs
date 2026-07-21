using System;
using System.Collections.Generic;
using System.Linq;

// Token: 0x020000AA RID: 170
public class Movement
{
	// Token: 0x06000ACA RID: 2762 RVA: 0x00036528 File Offset: 0x00034928
	public Direction EffectiveDir()
	{
		switch (this.movetype)
		{
		case Movement.MoveType.None:
			return Direction.None;
		case Movement.MoveType.Translation:
			return this.direction;
		case Movement.MoveType.Rotation:
			return DirectionUtil.ContinueRot(this.from, this.to);
		case Movement.MoveType.Pivot:
			return this.direction;
		default:
			return Direction.None;
		}
	}

	// Token: 0x170003D0 RID: 976
	// (get) Token: 0x06000ACB RID: 2763 RVA: 0x0003657A File Offset: 0x0003497A
	// (set) Token: 0x06000ACC RID: 2764 RVA: 0x00036582 File Offset: 0x00034982
	public int speed { get; private set; }

	// Token: 0x06000ACD RID: 2765 RVA: 0x0003658B File Offset: 0x0003498B
	public void SetSpeed(int _speed)
	{
		this.speed = _speed;
		this.remaining = new Fraction(1, 1 << _speed - 1);
	}

	// Token: 0x170003D1 RID: 977
	// (get) Token: 0x06000ACE RID: 2766 RVA: 0x000365A8 File Offset: 0x000349A8
	public bool translation
	{
		get
		{
			return this.movetype == Movement.MoveType.Translation;
		}
	}

	// Token: 0x170003D2 RID: 978
	// (get) Token: 0x06000ACF RID: 2767 RVA: 0x000365B3 File Offset: 0x000349B3
	public bool rotation
	{
		get
		{
			return this.movetype == Movement.MoveType.Rotation;
		}
	}

	// Token: 0x170003D3 RID: 979
	// (get) Token: 0x06000AD0 RID: 2768 RVA: 0x000365BE File Offset: 0x000349BE
	public bool pivot
	{
		get
		{
			return this.movetype == Movement.MoveType.Pivot;
		}
	}

	// Token: 0x06000AD1 RID: 2769 RVA: 0x000365CC File Offset: 0x000349CC
	public void Resolve()
	{
		switch (this.movetype)
		{
		case Movement.MoveType.Translation:
		{
			this.target.pos = this.target.pos + this.direction;
			this.target.CalcRoughOccupancyBounds();
			GameState gamestate = this.target.gamestate;
			if (gamestate.exitAttachment == this.target)
			{
				gamestate.exitPos += this.direction;
				if (this.torsion != 0)
				{
					gamestate.exitUp = !gamestate.exitUp;
					if (gamestate.exitDir.OrthoTo(this.target.direction))
					{
						gamestate.exitDir = gamestate.exitDir.Inverse();
					}
				}
			}
			break;
		}
		case Movement.MoveType.Rotation:
			if (this.target.type == EntType.sausage)
			{
				Entity entity = this.target.LadenTarget();
				if (entity != null && entity.type == EntType.fork)
				{
					if (this.target.direction.LeftOf(this.to))
					{
						entity.direction = entity.direction.RotCounterclockwise45();
					}
					else
					{
						entity.direction = entity.direction.RotClockwise45();
					}
					if (!(entity.pos == this.target.pos))
					{
						entity.pos = this.target.pos + this.to;
						this.target.CalcRoughOccupancyBounds();
					}
				}
				GameState gamestate2 = this.target.gamestate;
				if (gamestate2.exitAttachment == this.target)
				{
					if (this.target.direction.LeftOf(this.to))
					{
						gamestate2.exitDir = gamestate2.exitDir.RotCounterclockwise45();
					}
					else
					{
						gamestate2.exitDir = gamestate2.exitDir.RotClockwise45();
					}
					if (!(gamestate2.exitPos == this.target.pos))
					{
						gamestate2.exitPos = this.target.pos + this.to + Direction.Up;
						this.target.CalcRoughOccupancyBounds();
					}
				}
			}
			this.target.direction = this.to;
			break;
		case Movement.MoveType.Pivot:
			if (this.target.type == EntType.sausage)
			{
				Entity entity2 = this.target.LadenTarget();
				if (entity2 != null && entity2.type == EntType.fork)
				{
					if (this.target.direction.LeftOf(this.to))
					{
						entity2.direction = entity2.direction.RotCounterclockwise45();
					}
					else
					{
						entity2.direction = entity2.direction.RotClockwise45();
					}
					if (!(entity2.pos == this.target.pos))
					{
						entity2.pos = this.target.pos + this.to;
					}
					entity2.pos += this.direction;
				}
			}
			this.target.pos = this.target.pos + this.direction;
			this.target.direction = this.to;
			this.target.CalcRoughOccupancyBounds();
			break;
		}
		if (this.target.type == EntType.sausage)
		{
			if (this.target.dat.Length == 0 || this.target.dat[0] == 'M')
			{
				if (this.target.gamestate.pushestotry >= 0)
				{
					if (this.target.gamestate.pushestotry > 0)
					{
						if (this.target.pos.z < -2)
						{
							if (this.target.pos.z == -3)
							{
								Coord coord = this.target.pos + this.target.direction;
								Coord[] array = new Coord[20];
								for (int i = 0; i < 10; i++)
								{
									array[2 * i] = new Coord(this.target.pos.x, this.target.pos.y, this.target.pos.z - i);
									array[2 * i + 1] = new Coord(coord.x, coord.y, coord.z - i);
								}
								List<Entity> list = this.target.gamestate.EntsAt(array, false);
								if (list.Any<Entity>((Entity e) => e.type == EntType.island))
								{
									if (this.target.dat.Length == 0)
									{
										this.target.dat = "S ; ; ";
									}
									else
									{
										this.target.dat = 'S' + this.target.dat.Substring(1);
									}
								}
								else
								{
									if (this.target.dat.Length == 0)
									{
										this.target.dat = "L ; ; ";
									}
									else
									{
										this.target.dat = 'L' + this.target.dat.Substring(1);
									}
									this.target.gamestate.sausagelost = true;
								}
							}
							if (this.target.dat.Length == 0)
							{
								this.target.dat = "L ; ; ";
								this.target.gamestate.sausagelost = true;
							}
						}
					}
					else if (this.target.pos.z == -3)
					{
						if (this.target.dat.Length > 0)
						{
							this.target.dat = 'L' + this.target.dat.Substring(1);
							this.target.gamestate.sausagelost = true;
							this.target.pos = Coord.Invalid;
							if (this.target.gamestate.fork != null && this.target.stuckto == this.target.gamestate.fork.id)
							{
								this.target.gamestate.fork.pos = Coord.Invalid;
							}
						}
						else
						{
							this.target.dat = "L ; ; ";
							this.target.gamestate.sausagelost = true;
							this.target.pos = Coord.Invalid;
							if (this.target.gamestate.fork != null)
							{
								this.target.gamestate.fork.pos = Coord.Invalid;
							}
						}
					}
				}
			}
		}
		else if (this.target.type == EntType.fork && this.target.pos.z < -3)
		{
			this.target.pos = Coord.Invalid;
			this.target.CalcRoughOccupancyBounds();
		}
		if (this.target.type == EntType.sausage && !this.target.Laden())
		{
			Entity fork = this.target.gamestate.fork;
			bool flag = fork != null && fork.direction == this.direction.Inverse() && (fork.pos == this.target.pos || fork.pos == this.target.pos + this.target.direction);
			if (flag && fork.stuckto == -1 && fork.movement == null)
			{
				fork.stuckto = this.target.id;
				this.target.stuckto = fork.id;
				this.target.gamestate.forkfork = true;
				this.target.gamestate.forksfx = true;
			}
		}
		if (this.translation && !this.direction.Vertical())
		{
			this.target.gamestate.BuildBox(this.target);
		}
	}

	// Token: 0x06000AD2 RID: 2770 RVA: 0x00036E65 File Offset: 0x00035265
	public void Tick(Fraction deltaTime)
	{
		this.remaining -= deltaTime;
	}

	// Token: 0x06000AD3 RID: 2771 RVA: 0x00036E79 File Offset: 0x00035279
	public bool Done()
	{
		return this.remaining == 0;
	}

	// Token: 0x06000AD4 RID: 2772 RVA: 0x00036E87 File Offset: 0x00035287
	public bool Starting()
	{
		return this.remaining.num * (1 << this.speed - 1) == this.remaining.den;
	}

	// Token: 0x06000AD5 RID: 2773 RVA: 0x00036EB0 File Offset: 0x000352B0
	public static Movement Translation(Entity _target, Direction _direction, int _torsion, int _speed, Movement.MType _mtype = Movement.MType.Idle, bool _left = true)
	{
		Movement movement = _target._movement;
		movement.movetype = Movement.MoveType.Translation;
		movement.target = _target;
		movement.direction = _direction;
		if (_target == null)
		{
			movement.targetid = -1;
			movement.torsion = 0;
		}
		else
		{
			movement.targetid = _target.id;
			movement.torsion = ((!_direction.ParallelTo(_target.direction)) ? _torsion : 0);
		}
		movement.remaining = new Fraction(1, 1 << _speed - 1);
		movement.speed = _speed;
		movement.mtype = _mtype;
		movement.left = _left;
		movement.towerlevel = _target.gamestate.curtowerlevel;
		return movement;
	}

	// Token: 0x06000AD6 RID: 2774 RVA: 0x00036F5C File Offset: 0x0003535C
	public static Movement Fixed(Entity _target, int _speed)
	{
		Movement @fixed = Movement._Fixed;
		@fixed.target = _target;
		@fixed.targetid = _target.id;
		@fixed.movetype = Movement.MoveType.None;
		@fixed.direction = Direction.None;
		@fixed.torsion = 0;
		@fixed.from = Direction.None;
		@fixed.to = Direction.None;
		@fixed.left = false;
		@fixed.mtype = Movement.MType.Fixed;
		@fixed.speed = _speed;
		@fixed.remaining = new Fraction(1, 1 << _speed - 1);
		@fixed.towerlevel = -1;
		return @fixed;
	}

	// Token: 0x06000AD7 RID: 2775 RVA: 0x00036FD8 File Offset: 0x000353D8
	public static Movement Surprise(Entity _target, Direction _direction, Movement.MType _mtype)
	{
		Movement movement = _target._movement;
		movement.movetype = Movement.MoveType.None;
		movement.target = _target;
		if (_target == null)
		{
			movement.targetid = -1;
		}
		else
		{
			movement.targetid = _target.id;
		}
		movement.direction = _direction;
		movement.speed = 2;
		movement.mtype = _mtype;
		movement.remaining = new Fraction(1, 1);
		movement.speed = 1;
		movement.towerlevel = -1;
		return movement;
	}

	// Token: 0x06000AD8 RID: 2776 RVA: 0x0003704C File Offset: 0x0003544C
	public static Movement Rotation(Entity _target, Direction _from, Direction _to, Movement.MType _mtype, int _speed)
	{
		Movement movement = _target._movement;
		movement.movetype = Movement.MoveType.Rotation;
		movement.target = _target;
		movement.targetid = _target.id;
		movement.from = _from;
		movement.to = _to;
		movement.remaining = new Fraction(1, 1 << _speed - 1);
		movement.speed = _speed;
		movement.direction = Direction.None;
		movement.left = _to.LeftOf(_from);
		movement.mtype = _mtype;
		movement.towerlevel = -1;
		return movement;
	}

	// Token: 0x06000AD9 RID: 2777 RVA: 0x000370C8 File Offset: 0x000354C8
	public static Movement Pivot(Entity _target, Direction _direction, Direction _from, Direction _to, Movement.MType _mtype, int _speed)
	{
		Movement movement = _target._movement;
		movement.movetype = Movement.MoveType.Pivot;
		movement.direction = _direction;
		movement.target = _target;
		movement.targetid = _target.id;
		movement.from = _from;
		movement.to = _to;
		movement.remaining = new Fraction(1, 1 << _speed - 1);
		movement.speed = _speed;
		movement.left = _to.LeftOf(_from);
		movement.mtype = _mtype;
		movement.towerlevel = -1;
		return movement;
	}

	// Token: 0x06000ADA RID: 2778 RVA: 0x00037145 File Offset: 0x00035545
	public bool Rotating()
	{
		return this.direction == Direction.None && this.speed > 0;
	}

	// Token: 0x06000ADB RID: 2779 RVA: 0x0003715F File Offset: 0x0003555F
	public Movement SaveToMovement()
	{
		return this;
	}

	// Token: 0x06000ADC RID: 2780 RVA: 0x00037164 File Offset: 0x00035564
	public string SaveToString()
	{
		return string.Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12}, |", new object[]
		{
			this.target.id,
			(int)this.direction,
			this.torsion,
			this.remaining.num,
			this.remaining.den,
			this.speed,
			(int)this.movetype,
			(int)this.from,
			(int)this.to,
			(int)this.mtype,
			this.left,
			this.towerlevel,
			this.pureDirectForce
		});
	}

	// Token: 0x06000ADD RID: 2781 RVA: 0x0003724B File Offset: 0x0003564B
	public override string ToString()
	{
		return this.ToReadibleString(this.target.gamestate);
	}

	// Token: 0x06000ADE RID: 2782 RVA: 0x00037260 File Offset: 0x00035660
	public string ToShortReadibleString(GameState gs)
	{
		string text;
		if (this.movetype == Movement.MoveType.Translation)
		{
			text = string.Format("T {0} ({4} {2})", new object[]
			{
				gs.FromIDDynamic(this.target.id).QuickName(),
				this.direction,
				this.torsion,
				this.remaining.ToString(),
				this.speed,
				this.movetype,
				this.mtype,
				this.target.type,
				this.pureDirectForce,
				this.target.id
			});
		}
		else if (this.movetype == Movement.MoveType.Rotation)
		{
			text = string.Format("R {0} ({1} {2})", new object[]
			{
				gs.FromIDDynamic(this.target.id).type,
				this.from,
				this.to,
				this.remaining.ToString(),
				this.speed,
				this.mtype
			});
		}
		else if (this.movetype == Movement.MoveType.None)
		{
			text = string.Format("F {0}", new object[]
			{
				gs.FromIDDynamic(this.target.id).QuickName(),
				this.from,
				this.to,
				this.remaining.ToString(),
				this.speed,
				this.mtype,
				this.target.id
			});
		}
		else
		{
			text = string.Format("P {0} - {6}\n\tposition : {1}\n\tdirection : {2}\n\ttorsion : {3}\n\tremaining : {4}\n\tspeed : {5} ", new object[]
			{
				gs.FromIDDynamic(this.target.id).type,
				this.target.pos,
				this.direction,
				this.torsion,
				this.remaining.ToString(),
				this.speed,
				this.mtype
			});
			text += string.Format("\n\tfrom : {1}\n\to : {2}\n\tremaining : {3}\n\tspeed : {4}", new object[]
			{
				gs.FromIDDynamic(this.target.id).type,
				this.from,
				this.to,
				this.remaining.ToString(),
				this.speed
			});
		}
		return text;
	}

	// Token: 0x06000ADF RID: 2783 RVA: 0x00037560 File Offset: 0x00035960
	public string ToReadibleString(GameState gs)
	{
		string text;
		if (this.movetype == Movement.MoveType.Translation)
		{
			text = string.Format("T {0} - {6}\n\tdirection : {1}\n\ttorsion : {2}\n\tspeed : {4}\n\tmovetype : {5}\n\tremaining : {3}\n\tmtype : {6}\n\ttargettype : {7}\n\tpureDirectForce : {8}", new object[]
			{
				gs.FromIDDynamic(this.target.id).type,
				this.direction,
				this.torsion,
				this.remaining.ToString(),
				this.speed,
				this.movetype,
				this.mtype,
				this.target.type,
				this.pureDirectForce
			});
		}
		else if (this.movetype == Movement.MoveType.Rotation)
		{
			text = string.Format("R {0} - {5}\n\tfrom : {1}\n\tto : {2}\n\tremaining : {3}\n\tspeed : {4}", new object[]
			{
				gs.FromIDDynamic(this.target.id).type,
				this.from,
				this.to,
				this.remaining.ToString(),
				this.speed,
				this.mtype
			});
		}
		else
		{
			text = string.Format("P {0} - {6}\n\tposition : {1}\n\tdirection : {2}\n\ttorsion : {3}\n\tremaining : {4}\n\tspeed : {5} ", new object[]
			{
				gs.FromIDDynamic(this.target.id).type,
				this.target.pos,
				this.direction,
				this.torsion,
				this.remaining.ToString(),
				this.speed,
				this.mtype
			});
			text += string.Format("\n\tfrom : {1}\n\to : {2}\n\tremaining : {3}\n\tspeed : {4}", new object[]
			{
				gs.FromIDDynamic(this.target.id).type,
				this.from,
				this.to,
				this.remaining.ToString(),
				this.speed
			});
		}
		return text;
	}

	// Token: 0x06000AE0 RID: 2784 RVA: 0x000377B8 File Offset: 0x00035BB8
	public static Movement Load(string dat, GameState gs)
	{
		Movement movement = new Movement();
		string[] array = dat.Split(new char[] { ',' });
		int num = array[0].IntParseFast();
		movement.target = gs.FromIDDynamic(num);
		movement.direction = (Direction)array[1].IntParseFast();
		movement.torsion = array[2].IntParseFast();
		int num2 = array[3].IntParseFast();
		int num3 = array[4].IntParseFast();
		movement.remaining = new Fraction(num2, num3);
		movement.speed = array[5].IntParseFast();
		movement.movetype = (Movement.MoveType)array[6].IntParseFast();
		movement.from = (Direction)array[7].IntParseFast();
		movement.to = (Direction)array[8].IntParseFast();
		movement.mtype = (Movement.MType)array[9].IntParseFast();
		movement.left = bool.Parse(array[10]);
		movement.pureDirectForce = bool.Parse(array[11]);
		return movement;
	}

	// Token: 0x040006A2 RID: 1698
	public Entity target;

	// Token: 0x040006A3 RID: 1699
	public int targetid;

	// Token: 0x040006A4 RID: 1700
	public Movement.MoveType movetype;

	// Token: 0x040006A5 RID: 1701
	public Direction direction;

	// Token: 0x040006A6 RID: 1702
	public int torsion;

	// Token: 0x040006A7 RID: 1703
	public bool pureDirectForce;

	// Token: 0x040006A9 RID: 1705
	public Fraction remaining;

	// Token: 0x040006AA RID: 1706
	public Direction from;

	// Token: 0x040006AB RID: 1707
	public Direction to;

	// Token: 0x040006AC RID: 1708
	public Movement.MType mtype;

	// Token: 0x040006AD RID: 1709
	public bool left;

	// Token: 0x040006AE RID: 1710
	public int towerlevel;

	// Token: 0x040006AF RID: 1711
	private static Movement _Fixed = new Movement();

	// Token: 0x020000AB RID: 171
	public enum MType
	{
		// Token: 0x040006B2 RID: 1714
		Idle,
		// Token: 0x040006B3 RID: 1715
		WalkForward,
		// Token: 0x040006B4 RID: 1716
		ForwardPedal,
		// Token: 0x040006B5 RID: 1717
		Backpedal,
		// Token: 0x040006B6 RID: 1718
		TurnIn,
		// Token: 0x040006B7 RID: 1719
		TurnOut,
		// Token: 0x040006B8 RID: 1720
		TurnBackout,
		// Token: 0x040006B9 RID: 1721
		StrafeL,
		// Token: 0x040006BA RID: 1722
		StrafeR,
		// Token: 0x040006BB RID: 1723
		ClimbUp_Init,
		// Token: 0x040006BC RID: 1724
		ClimbUp_Loop,
		// Token: 0x040006BD RID: 1725
		ClimbUp_End1,
		// Token: 0x040006BE RID: 1726
		ClimbUp_End2,
		// Token: 0x040006BF RID: 1727
		ClimbDown_Init1,
		// Token: 0x040006C0 RID: 1728
		ClimbDown_Init2,
		// Token: 0x040006C1 RID: 1729
		ClimbDown_Loop,
		// Token: 0x040006C2 RID: 1730
		ClimbDown_End,
		// Token: 0x040006C3 RID: 1731
		Fall,
		// Token: 0x040006C4 RID: 1732
		NoCanDo,
		// Token: 0x040006C5 RID: 1733
		Surprise_Chasm,
		// Token: 0x040006C6 RID: 1734
		PivotIn,
		// Token: 0x040006C7 RID: 1735
		PivotOut,
		// Token: 0x040006C8 RID: 1736
		Fixed
	}

	// Token: 0x020000AC RID: 172
	public enum MoveType
	{
		// Token: 0x040006CA RID: 1738
		None,
		// Token: 0x040006CB RID: 1739
		Translation,
		// Token: 0x040006CC RID: 1740
		Rotation,
		// Token: 0x040006CD RID: 1741
		Pivot
	}
}
