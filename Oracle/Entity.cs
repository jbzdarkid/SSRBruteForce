using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

// Token: 0x0200009B RID: 155
public class Entity
{
	// Token: 0x06000969 RID: 2409 RVA: 0x000206F8 File Offset: 0x0001EAF8
	public Entity(GameState _gamestate)
	{
		this.gamestate = _gamestate;
		if (_gamestate != null)
		{
			this.id = _gamestate.GetNewID();
		}
		this.stuckto = -1;
		this.cookdata = 0;
		this.dat = string.Empty;
	}

	// Token: 0x0600096A RID: 2410 RVA: 0x000207AE File Offset: 0x0001EBAE
	public void SetParent(GameState gs)
	{
		this.gamestate = gs;
	}

	// Token: 0x0600096B RID: 2411 RVA: 0x000207B7 File Offset: 0x0001EBB7
	public bool IsPedastal()
	{
		return this.type == EntType.ground && this.tileset == 12 && this.tilenum == 9;
	}

	// Token: 0x0600096C RID: 2412 RVA: 0x000207E0 File Offset: 0x0001EBE0
	public int FootprintType(int islandtileset)
	{
		if (this.type == EntType.ground || this.type == EntType.ladder)
		{
			switch (islandtileset)
			{
			case 0:
				if (this.tileset == 2 || this.tileset == 3)
				{
					return 15;
				}
				if (this.tileset == 5 || this.tileset == 10 || (this.tileset == 11 && this.tilenum >= 5))
				{
					return 16;
				}
				return 1;
			case 1:
				return 13;
			case 2:
				if ((this.tileset == 0 && this.tilenum >= 0 && this.tilenum <= 4) || (this.tileset == 1 && this.tilenum >= 0 && this.tilenum <= 2) || (this.tileset == 2 && this.tilenum >= 0 && this.tilenum <= 1) || (this.tileset == 5 && this.tilenum >= 0 && this.tilenum <= 0) || (this.tileset == 7 && this.tilenum >= 0 && this.tilenum <= 2) || (this.tileset == 10 && this.tilenum >= 2 && this.tilenum <= 5) || (this.tileset == 11 && this.tilenum >= 2 && this.tilenum <= 5) || (this.tileset == 12 && this.tilenum >= 3 && this.tilenum <= 5))
				{
					return 14;
				}
				if (this.tileset == 2 || this.tileset == 3)
				{
					return 15;
				}
				if (this.tileset == 5 || this.tileset == 10 || this.tileset == 11)
				{
					return 16;
				}
				return 1;
			case 3:
				return 18;
			case 4:
				if ((this.tileset == 7 && this.tilenum == 0 && this.type == EntType.ladder) || (this.tileset == 8 && this.tilenum == 0 && this.type == EntType.ladder) || (this.tileset == 9 && this.tilenum == 0 && this.type == EntType.ladder) || (this.tileset == 10 && this.tilenum >= 0 && this.tilenum <= 5))
				{
					return 1;
				}
				if (this.tileset == 11 && this.tilenum >= 0 && this.tilenum <= 3)
				{
					return 15;
				}
				return 17;
			}
		}
		else
		{
			if (this.type == EntType.sausage)
			{
				return 19;
			}
			if (this.type == EntType.bbq)
			{
				if (this.gamestate.bbqsOn())
				{
					return 20;
				}
				return 21;
			}
		}
		return 0;
	}

	// Token: 0x0600096D RID: 2413 RVA: 0x00020ADC File Offset: 0x0001EEDC
	public int DecorationType()
	{
		if (this.gamestate.tileset == 0)
		{
			if (this.tileset == 5)
			{
				if (this.tilenum == 6)
				{
					return 14;
				}
				if (this.tilenum == 9)
				{
					return 15;
				}
			}
			if (this.tileset == 7 && this.tilenum > 6)
			{
				return -1;
			}
			if (this.tileset == 8 && this.tilenum >= 4)
			{
				return -1;
			}
		}
		if (this.gamestate.tileset == 2 && this.tileset == 8 && this.tilenum > 3)
		{
			return -1;
		}
		if (this.gamestate.tileset == 0 || this.gamestate.tileset == 1 || (this.gamestate.tileset == 2 && this.tilenum <= 6))
		{
			if (this.tileset == 7 || this.tileset == 8)
			{
				return (this.tileset - 7) * 10 + this.tilenum;
			}
			return -1;
		}
		else
		{
			if (this.gamestate.tileset == 4 && this.tileset == 7 && this.tilenum <= 3)
			{
				return (this.tileset - 7) * 10 + this.tilenum;
			}
			return -1;
		}
	}

	// Token: 0x0600096E RID: 2414 RVA: 0x00020C2C File Offset: 0x0001F02C
	public bool Decoration()
	{
		if (this.type == EntType.ground)
		{
			if (this.gamestate.tileset == 4)
			{
				if ((this.tileset == 7 || this.tileset == 8 || this.tileset == 9) && this.tilenum >= 4 && this.tilenum <= 7)
				{
					return false;
				}
				if (this.tileset == 9 && this.tilenum < 4)
				{
					return false;
				}
			}
			return (this.gamestate.tileset == 0 && (this.tileset == 5 || this.tileset == 13) && this.tilenum >= 6) || (this.type == EntType.ground && (this.tileset == 7 || this.tileset == 8 || this.tileset == 9));
		}
		return false;
	}

	// Token: 0x0600096F RID: 2415 RVA: 0x00020D1C File Offset: 0x0001F11C
	public void Pivot()
	{
		if (this.type != EntType.sausage)
		{
			return;
		}
		if (this.movement != null)
		{
			Debug.LogError("trying to pivot moving object");
		}
		this.pos += this.direction;
		this.direction = this.direction.Inverse();
		this.pivot = 1 - this.pivot;
		string[] array = this.dat.Split(new char[] { ';' });
		if (array.Length == 3)
		{
			this.dat = string.Concat(new string[]
			{
				array[0],
				";",
				array[2],
				";",
				array[1]
			});
		}
		int[] array2 = new int[]
		{
			this.cookdata % 4,
			this.cookdata / 4 % 4,
			this.cookdata / 16 % 4,
			this.cookdata / 64 % 4
		};
		this.cookdata = array2[3] + 4 * array2[2] + 16 * array2[1] + 64 * array2[0];
	}

	// Token: 0x06000970 RID: 2416 RVA: 0x00020E2C File Offset: 0x0001F22C
	public bool Pushable()
	{
		return !this.type.Static() && !this.gamestate.overworld && (this.type != EntType.island || !(this.dat == this.gamestate.pushtargetlevel));
	}

	// Token: 0x06000971 RID: 2417 RVA: 0x00020E84 File Offset: 0x0001F284
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

	// Token: 0x06000972 RID: 2418 RVA: 0x00020F24 File Offset: 0x0001F324
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

	// Token: 0x06000973 RID: 2419 RVA: 0x00020FC4 File Offset: 0x0001F3C4
	public EntitySkeleton SaveToEntSkel()
	{
		if (this._lastskel == null || this.pos != this._lastskel.pos || this.direction != this._lastskel.direction || this.stuckto != this._lastskel.stuckto || this.rot != this._lastskel.rot || this.cookdata != this._lastskel.cookdata || this.turndir != this._lastskel.turndir || this.pivot != this._lastskel.pivot || this.dat != this._lastskel.dat)
		{
			this._lastskel = new EntitySkeleton(this);
		}
		return this._lastskel;
	}

	// Token: 0x06000974 RID: 2420 RVA: 0x000210A8 File Offset: 0x0001F4A8
	public Entity SaveToEnt()
	{
		Entity entity = new Entity(null);
		entity.CopyFrom(this);
		return entity;
	}

	// Token: 0x06000975 RID: 2421 RVA: 0x000210C4 File Offset: 0x0001F4C4
	public override string ToString()
	{
		return string.Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11}", new object[]
		{
			this.pos, this.direction, this.type, this.id, this.dat, this.stuckto, this.rot, this.cookdata, this.turndir, this.tilenum,
			this.tileset, this.pivot
		});
	}

	// Token: 0x06000976 RID: 2422 RVA: 0x00021188 File Offset: 0x0001F588
	public string SaveToString()
	{
		return string.Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},|", new object[]
		{
			this.pos.x,
			this.pos.y,
			this.pos.z,
			(int)this.type,
			this.id,
			(int)this.direction,
			this.dat,
			this.stuckto,
			this.rot,
			0,
			this.cookdata,
			(int)this.turndir,
			this.tilenum,
			this.tileset,
			this.pivot
		});
	}

	// Token: 0x06000977 RID: 2423 RVA: 0x00021284 File Offset: 0x0001F684
	public void Save(StringBuilder result)
	{
		result.Append(this.pos.x.ToStringFast());
		result.Append(',');
		result.Append(this.pos.y.ToStringFast());
		result.Append(',');
		result.Append(this.pos.z.ToStringFast());
		result.Append(',');
		result.Append(((int)this.type).ToStringFast());
		result.Append(',');
		result.Append(this.id.ToStringFast());
		result.Append(',');
		result.Append(((int)this.direction).ToStringFast());
		result.Append(',');
		result.Append(this.dat);
		result.Append(',');
		result.Append(this.stuckto.ToStringFast());
		result.Append(',');
		result.Append(this.rot.ToStringFast());
		result.Append(',');
		result.Append("0");
		result.Append(',');
		result.Append(this.cookdata.ToStringFast());
		result.Append(',');
		result.Append(((int)this.turndir).ToStringFast());
		result.Append(',');
		result.Append(this.tilenum.ToStringFast());
		result.Append(',');
		result.Append(this.tileset.ToStringFast());
		result.Append(',');
		result.Append(this.pivot.ToStringFast());
		result.Append(",|");
	}

	// Token: 0x06000978 RID: 2424 RVA: 0x00021430 File Offset: 0x0001F830
	public Entity.EntDat SaveToArray()
	{
		int[] array = new int[]
		{
			this.pos.x,
			this.pos.y,
			this.pos.z,
			(int)this.type,
			this.id,
			(int)this.direction,
			this.stuckto,
			this.rot,
			0,
			this.cookdata,
			(int)this.turndir,
			this.tilenum,
			this.tileset,
			this.pivot
		};
		return new Entity.EntDat(array, this.dat);
	}

	// Token: 0x06000979 RID: 2425 RVA: 0x000214DC File Offset: 0x0001F8DC
	public bool Extended()
	{
		EntType entType = this.type;
		if (entType != EntType.player)
		{
			return entType == EntType.sausage || entType == EntType.island;
		}
		return this.gamestate.fork == null;
	}

	// Token: 0x0600097A RID: 2426 RVA: 0x0002151C File Offset: 0x0001F91C
	public void TryRotate(Direction rot_dir)
	{
		if (this.direction.Valid() && rot_dir.ParallelTo(this.direction))
		{
			return;
		}
		this.rot = 1 - this.rot;
		Entity fork = this.gamestate.fork;
		if (fork != null && fork.id == this.stuckto && this.direction.OrthoTo(fork.direction))
		{
			fork.direction = fork.direction.Inverse();
		}
	}

	// Token: 0x0600097B RID: 2427 RVA: 0x000215A4 File Offset: 0x0001F9A4
	public Coord TargetPos()
	{
		if (this.movement == null)
		{
			return this.pos;
		}
		switch (this.movement.movetype)
		{
		case Movement.MoveType.Translation:
			return this.pos + this.movement.direction;
		}
		return this.pos;
	}

	// Token: 0x0600097C RID: 2428 RVA: 0x00021607 File Offset: 0x0001FA07
	public bool Turning()
	{
		return this.direction.Diagonal();
	}

	// Token: 0x0600097D RID: 2429 RVA: 0x00021614 File Offset: 0x0001FA14
	public static Entity LoadFromArray(Entity.EntDat ed, GameState gamestate, bool precalc = true)
	{
		Entity entity = new Entity(gamestate);
		int num = ed.arr[0];
		int num2 = ed.arr[1];
		int num3 = ed.arr[2];
		entity.pos = new Coord(num, num2, num3);
		int num4 = ed.arr[3];
		entity.type = (EntType)num4;
		entity.id = ed.arr[4];
		if (gamestate.NextID() < entity.id)
		{
			gamestate.SetIDCounterTo(entity.id + 1);
		}
		entity.direction = (Direction)ed.arr[5];
		entity.dat = ed.dat;
		if (entity.dat.Length == 0 && entity.type == EntType.ground)
		{
			entity.dat = "0";
		}
		entity.stuckto = ed.arr[6];
		int num5 = ed.arr[7];
		int num6 = ed.arr[8];
		entity.rot = ((num5 != 1 && num6 != 1) ? 0 : 1);
		entity.cookdata = ed.arr[9];
		entity.turndir = (Direction)ed.arr[10];
		entity.tilenum = ed.arr[11];
		entity.tileset = ed.arr[12];
		entity.pivot = ed.arr[13];
		if (precalc)
		{
			entity.PrecalcAll();
		}
		return entity;
	}

	// Token: 0x0600097E RID: 2430 RVA: 0x00021768 File Offset: 0x0001FB68
	public void PrecalcAll()
	{
		if (this.gamestate.metagame == null)
		{
			return;
		}
		if (this.type == EntType.island)
		{
			this.occ_oldpos = Coord.Invalid;
			this.occ_olddir = -1;
			this.occ_oldcookdat = -100;
			this.islandoccupancymaskcalculated = false;
			this.mask = null;
			this.hiddenfootprintcalculated = false;
			this.cachedsourcefootprint = null;
			this.lastsourcepos = Coord.Invalid;
			this.lastsourceposlowered = Coord.Invalid;
			Entity.IslandOccupanciesRepository.Remove(this.dat);
			Entity.IslandOccupanciesRepository.Remove(this.dat + "_Enter");
			Entity.IslandOccupanciesRepository.Remove(this.dat + "_Exit");
			Entity.IslandOccupanciesRepository.Remove(this.dat + "_Completed");
			Entity.IslandOccupanciesRepository.Remove(this.dat + "_Enter_Completed");
			Entity.IslandOccupanciesRepository.Remove(this.dat + "_Exit_Completed");
			this.CalcIslandOccupancyMask();
			this.CalcHiddenFootprint();
			this.IslandSourceFootprint();
			this.IslandSourceFootprintLower();
		}
		this.Occupancy();
	}

	// Token: 0x0600097F RID: 2431 RVA: 0x00021898 File Offset: 0x0001FC98
	public static Entity LoadFromArrayInline(string source, List<int> tokens, GameState gamestate, bool precalc = true)
	{
		Entity entity = new Entity(gamestate);
		int num = source.IntParseFast(0, tokens[0]);
		int num2 = source.IntParseFast(tokens[0] + 1, tokens[1]);
		int num3 = source.IntParseFast(tokens[1] + 1, tokens[2]);
		entity.pos = new Coord(num, num2, num3);
		int num4 = source.IntParseFast(tokens[2] + 1, tokens[3]);
		entity.type = (EntType)num4;
		entity.id = source.IntParseFast(tokens[3] + 1, tokens[4]);
		if (gamestate.NextID() < entity.id)
		{
			gamestate.SetIDCounterTo(entity.id + 1);
		}
		entity.direction = (Direction)source.IntParseFast(tokens[4] + 1, tokens[5]);
		if (entity.type == EntType.ground && tokens[6] == tokens[5] + 1)
		{
			entity.dat = "0";
		}
		else
		{
			entity.dat = source.Substring(tokens[5] + 1, tokens[6] - tokens[5] - 1);
		}
		entity.stuckto = source.IntParseFast(tokens[6] + 1, tokens[7]);
		int num5 = source.IntParseFast(tokens[7] + 1, tokens[8]);
		int num6 = source.IntParseFast(tokens[8] + 1, tokens[9]);
		entity.rot = ((num5 != 1 && num6 != 1) ? 0 : 1);
		entity.cookdata = source.IntParseFast(tokens[9] + 1, tokens[10]);
		entity.turndir = (Direction)source.IntParseFast(tokens[10] + 1, tokens[11]);
		entity.tilenum = source.IntParseFast(tokens[11] + 1, tokens[12]);
		entity.tileset = source.IntParseFast(tokens[12] + 1, tokens[13]);
		if (tokens.Count > 15)
		{
			entity.pivot = source.IntParseFast(tokens[13] + 1, tokens[14]);
		}
		if (precalc)
		{
			entity.PrecalcAll();
		}
		return entity;
	}

	// Token: 0x06000980 RID: 2432 RVA: 0x00021AE0 File Offset: 0x0001FEE0
	public static Entity LoadFromArray(string[] tokens, GameState gamestate, bool precalc = true)
	{
		Entity entity = new Entity(gamestate);
		int num = tokens[0].IntParseFast();
		int num2 = tokens[1].IntParseFast();
		int num3 = tokens[2].IntParseFast();
		entity.pos = new Coord(num, num2, num3);
		int num4 = tokens[3].IntParseFast();
		entity.type = (EntType)num4;
		entity.id = tokens[4].IntParseFast();
		if (gamestate.NextID() < entity.id)
		{
			gamestate.SetIDCounterTo(entity.id + 1);
		}
		entity.direction = (Direction)tokens[5].IntParseFast();
		entity.dat = tokens[6];
		if (entity.dat.Length == 0 && entity.type == EntType.ground)
		{
			entity.dat = "0";
		}
		entity.stuckto = tokens[7].IntParseFast();
		int num5 = tokens[8].IntParseFast();
		int num6 = tokens[9].IntParseFast();
		entity.rot = ((num5 != 1 && num6 != 1) ? 0 : 1);
		entity.cookdata = tokens[10].IntParseFast();
		entity.turndir = (Direction)tokens[11].IntParseFast();
		entity.tilenum = tokens[12].IntParseFast();
		entity.tileset = tokens[13].IntParseFast();
		if (tokens.Length > 15)
		{
			entity.pivot = tokens[14].IntParseFast();
		}
		if (precalc)
		{
			entity.PrecalcAll();
		}
		return entity;
	}

	// Token: 0x06000981 RID: 2433 RVA: 0x00021C3B File Offset: 0x0002003B
	public static Entity Load(string dat, GameState gamestate, bool precalc = true)
	{
		ParseUtils.FindIndices(dat, ',', Entity._EntDatIndices);
		return Entity.LoadFromArrayInline(dat, Entity._EntDatIndices, gamestate, precalc);
	}

	// Token: 0x06000982 RID: 2434 RVA: 0x00021C58 File Offset: 0x00020058
	public int Bottom()
	{
		if (this.type == EntType.island)
		{
			if (this.mask == null)
			{
				this.mask = this.gamestate.metagame.islandmasks[this.dat];
			}
			return this.pos.z + this.mask.offset.z;
		}
		return this.pos.z;
	}

	// Token: 0x06000983 RID: 2435 RVA: 0x00021CC8 File Offset: 0x000200C8
	public int Top()
	{
		if (this.type == EntType.island)
		{
			if (this.mask == null)
			{
				this.mask = this.gamestate.metagame.islandmasks[this.dat];
			}
			int num = ((this.mask.mask.Length != 0 && this.mask.mask[0].Length != 0) ? this.mask.mask[0][0].Length : 0);
			return this.pos.z + this.mask.offset.z + num;
		}
		return this.pos.z;
	}

	// Token: 0x06000984 RID: 2436 RVA: 0x00021D80 File Offset: 0x00020180
	private bool IslandAt(Coord _pos, bool weak = false)
	{
		if (this.mask == null)
		{
			this.mask = this.gamestate.metagame.islandmasks[this.dat];
		}
		Coord coord = _pos - this.pos - this.mask.offset;
		if (coord.x < 0 || coord.y < 0 || coord.z < 0)
		{
			return false;
		}
		int num = this.mask.mask.Length;
		if (coord.x >= num)
		{
			return false;
		}
		int num2 = ((num != 0) ? this.mask.mask[0].Length : 0);
		if (coord.y >= num2)
		{
			return false;
		}
		int num3 = ((num2 != 0) ? this.mask.mask[0][0].Length : 0);
		if (coord.z >= num3)
		{
			return false;
		}
		if (weak)
		{
			return this.mask.mask[coord.x][coord.y][coord.z] != 0;
		}
		if (this.cookdata == 0)
		{
			int num4 = this.mask.mask[coord.x][coord.y][coord.z];
			return num4 > 0 || num4 == -1;
		}
		return this.mask.mask[coord.x][coord.y][coord.z] > 0;
	}

	// Token: 0x06000985 RID: 2437 RVA: 0x00021F0C File Offset: 0x0002030C
	private bool IslandAt(Coord[] _pos)
	{
		if (this.mask == null)
		{
			this.mask = this.gamestate.metagame.islandmasks[this.dat];
		}
		int num = this.pos.x + this.mask.offset.x;
		int num2 = this.pos.y + this.mask.offset.y;
		int num3 = this.pos.z + this.mask.offset.z;
		int num4 = this.mask.mask.Length;
		int num5 = ((num4 != 0) ? this.mask.mask[0].Length : 0);
		int num6 = ((num5 != 0) ? this.mask.mask[0][0].Length : 0);
		int num7 = num + num4;
		int num8 = num2 + num5;
		int num9 = num3 + num6;
		foreach (Coord coord in _pos)
		{
			if (coord.x >= num && coord.y >= num2 && coord.z >= num3 && coord.x < num7 && coord.y < num8 && coord.z < num9)
			{
				Coord coord2 = coord - this.pos - this.mask.offset;
				if (this.cookdata == 0)
				{
					int num10 = this.mask.mask[coord2.x][coord2.y][coord2.z];
					if (num10 > 0 || num10 == -1)
					{
						return true;
					}
				}
				else if (this.mask.mask[coord2.x][coord2.y][coord2.z] > 0)
				{
					return true;
				}
			}
		}
		return false;
	}

	// Token: 0x06000986 RID: 2438 RVA: 0x00022108 File Offset: 0x00020508
	public bool At(Coord[] _pos, bool instant, bool recurse = true)
	{
		if (this.type != EntType.island)
		{
			foreach (Coord coord in _pos)
			{
				if (coord == this.pos)
				{
					return true;
				}
			}
			bool flag = this.Extended();
			if (flag)
			{
				Coord coord2 = this.pos + this.direction;
				foreach (Coord coord3 in _pos)
				{
					if (coord3 == coord2)
					{
						return true;
					}
				}
			}
			if (this.movement != null && (!instant || !this.movement.Starting()) && !this.movement.direction.Invalid())
			{
				if (this.movement.translation)
				{
					Coord coord4 = this.pos + this.movement.direction;
					foreach (Coord coord5 in _pos)
					{
						if (coord5 == coord4)
						{
							return true;
						}
					}
					if (this.Extended())
					{
						coord4 = this.pos + this.direction + this.movement.direction;
						foreach (Coord coord6 in _pos)
						{
							if (coord6 == coord4)
							{
								return true;
							}
						}
					}
				}
				else
				{
					Coord coord7 = this.pos + this.movement.to;
					foreach (Coord coord8 in _pos)
					{
						if (coord8 == coord7)
						{
							return true;
						}
					}
				}
			}
			return false;
		}
		if (this.IslandAt(_pos))
		{
			return true;
		}
		if (this.movement == null || !this.movement.translation)
		{
			return false;
		}
		if ((!instant || !this.movement.Starting()) && !this.movement.direction.Invalid())
		{
			this.pos += this.movement.direction;
			bool flag2 = this.IslandAt(_pos);
			this.pos -= this.movement.direction;
			return flag2;
		}
		return false;
	}

	// Token: 0x06000987 RID: 2439 RVA: 0x000223B0 File Offset: 0x000207B0
	public bool At(Coord _pos, bool instant, bool recurse = true, bool weak = false)
	{
		if (this.type == EntType.island)
		{
			if (this.IslandAt(_pos, weak))
			{
				return true;
			}
		}
		else
		{
			if (_pos == this.pos)
			{
				return true;
			}
			bool flag = this.Extended();
			if (flag && _pos == this.pos + this.direction)
			{
				return true;
			}
		}
		if (this.movement != null && this.movement.target == this && (!instant || !this.movement.Starting()) && !this.movement.direction.Invalid())
		{
			if (this.movement.translation)
			{
				if (this.type == EntType.island)
				{
					if (recurse && this.At(_pos - this.movement.direction, instant, false, false))
					{
						return true;
					}
				}
				else
				{
					if (_pos == this.pos + this.movement.direction)
					{
						return true;
					}
					if (this.Extended() && _pos == this.pos + this.direction + this.movement.direction)
					{
						return true;
					}
				}
			}
			else if (this.pos + this.movement.to == _pos)
			{
				return true;
			}
		}
		return false;
	}

	// Token: 0x06000988 RID: 2440 RVA: 0x00022530 File Offset: 0x00020930
	public bool TargetIs(Entity e)
	{
		if (this.direction < Direction.North)
		{
			Debug.LogError(e.type + "," + this.type);
			return false;
		}
		Coord coord = this.pos + this.direction.ToCoord();
		return e.pos == coord;
	}

	// Token: 0x06000989 RID: 2441 RVA: 0x00022594 File Offset: 0x00020994
	public int IslandMaskVal(Coord _pos, out Coord localcoord)
	{
		if (this.mask == null)
		{
			this.mask = this.gamestate.metagame.islandmasks[this.dat];
		}
		localcoord = _pos - this.pos - this.mask.offset;
		if (localcoord.x >= 0 && localcoord.y >= 0 && localcoord.z >= 0 && localcoord.x < this.mask.mask.Length && localcoord.y < this.mask.mask[0].Length && localcoord.z < this.mask.mask[0][0].Length)
		{
			return this.mask.mask[localcoord.x][localcoord.y][localcoord.z];
		}
		return 0;
	}

	// Token: 0x0600098A RID: 2442 RVA: 0x00022684 File Offset: 0x00020A84
	public int IslandMaskVal(Coord _pos)
	{
		if (this.mask == null)
		{
			this.mask = this.gamestate.metagame.islandmasks[this.dat];
		}
		Coord coord = _pos - this.pos - this.mask.offset;
		if (coord.x >= 0 && coord.y >= 0 && coord.z >= 0 && coord.x < this.mask.mask.Length && coord.y < this.mask.mask[0].Length && coord.z < this.mask.mask[0][0].Length)
		{
			return this.mask.mask[coord.x][coord.y][coord.z];
		}
		return 0;
	}

	// Token: 0x0600098B RID: 2443 RVA: 0x00022778 File Offset: 0x00020B78
	private void CalcHiddenFootprint()
	{
		if (this.hiddenfootprintcalculated)
		{
			return;
		}
		if (this.gamestate.metagame == null)
		{
			return;
		}
		if (this.mask == null && !this.gamestate.metagame.islandmasks.TryGetValue(this.dat, out this.mask))
		{
			this.mask = new IslandMask(new int[0][][], Coord.Zero);
		}
		int num = this.mask.mask.Length;
		List<Coord> list = new List<Coord>(num);
		List<Coord> list2 = new List<Coord>(num / 2);
		int num2 = this.mask.mask.Length;
		int num3 = ((num2 != 0) ? this.mask.mask[0].Length : 0);
		int num4 = ((num3 != 0) ? this.mask.mask[0][0].Length : 0);
		for (int i = 0; i < num2; i++)
		{
			for (int j = 0; j < num3; j++)
			{
				for (int k = 0; k < num4; k++)
				{
					if (this.mask.mask[i][j][k] > 0 || this.mask.mask[i][j][k] == -1)
					{
						list.Add(this.mask.offset + new Coord(i, j, k));
						if (k == 0 || (this.mask.mask[i][j][k - 1] <= 0 && this.mask.mask[i][j][k] != -1))
						{
							list2.Add(this.mask.offset + new Coord(i, j, k - 1));
						}
					}
				}
			}
		}
		Entity.CoordComparer coordComparer = new Entity.CoordComparer();
		list.Sort(coordComparer);
		list2.Sort(coordComparer);
		this.hiddenfootprint = list.ToArray();
		this.hiddenfootprintlower = list2.ToArray();
		list = new List<Coord>(num);
		list2 = new List<Coord>(num / 2);
		for (int l = 0; l < num2; l++)
		{
			for (int m = 0; m < num3; m++)
			{
				for (int n = 0; n < num4; n++)
				{
					if (this.mask.mask[l][m][n] > 0)
					{
						list.Add(this.mask.offset + new Coord(l, m, n));
						if (n == 0 || this.mask.mask[l][m][n - 1] <= 0)
						{
							list2.Add(this.mask.offset + new Coord(l, m, n - 1));
						}
					}
				}
			}
		}
		this.hiddenfootprint_completed = list.ToArray();
		this.hiddenfootprintlower_completed = list2.ToArray();
		this.hiddenfootprintcalculated = true;
	}

	// Token: 0x0600098C RID: 2444 RVA: 0x00022A70 File Offset: 0x00020E70
	private Coord[] IslandSourceFootprint()
	{
		if (!this.hiddenfootprintcalculated)
		{
			this.CalcHiddenFootprint();
		}
		if (this.cookdata == 0)
		{
			if (this.cachedsourcefootprint == null)
			{
				this.cachedsourcefootprint = new Coord[this.hiddenfootprint.Length];
			}
			else if (this.cachedsourcefootprint.Length != this.hiddenfootprint.Length)
			{
				this.cachedsourcefootprint = new Coord[this.hiddenfootprint.Length];
			}
		}
		else if (this.cachedsourcefootprint == null)
		{
			this.cachedsourcefootprint = new Coord[this.hiddenfootprint_completed.Length];
		}
		else if (this.cachedsourcefootprint.Length != this.hiddenfootprint_completed.Length)
		{
			this.cachedsourcefootprint = new Coord[this.hiddenfootprint_completed.Length];
		}
		if (this.lastsourcepos != this.pos || this.lastsourcecooked != this.cookdata)
		{
			if (this.cookdata == 0)
			{
				for (int i = 0; i < this.hiddenfootprint.Length; i++)
				{
					this.cachedsourcefootprint[i] = this.hiddenfootprint[i] + this.pos;
				}
			}
			else
			{
				for (int j = 0; j < this.hiddenfootprint_completed.Length; j++)
				{
					this.cachedsourcefootprint[j] = this.hiddenfootprint_completed[j] + this.pos;
				}
			}
			this.lastsourcepos = this.pos;
			this.lastsourcecooked = this.cookdata;
		}
		return this.cachedsourcefootprint;
	}

	// Token: 0x0600098D RID: 2445 RVA: 0x00022C18 File Offset: 0x00021018
	private Coord[] IslandSourceFootprintLower()
	{
		if (!this.hiddenfootprintcalculated)
		{
			this.CalcHiddenFootprint();
		}
		if (this.cachedsourcefootprintlowered == null || this.cachedsourcefootprintlowered.Length < this.hiddenfootprintlower.Length)
		{
			this.cachedsourcefootprintlowered = new Coord[this.hiddenfootprintlower.Length];
		}
		if (this.lastsourceposlowered != this.pos || this.lastsourcecookedlowered != this.cookdata)
		{
			if (this.cookdata != 0)
			{
				for (int i = 0; i < this.hiddenfootprintlower.Length; i++)
				{
					this.cachedsourcefootprintlowered[i] = this.hiddenfootprintlower[i] + this.pos;
				}
			}
			else
			{
				for (int j = 0; j < this.hiddenfootprintlower_completed.Length; j++)
				{
					this.cachedsourcefootprintlowered[j] = this.hiddenfootprintlower_completed[j] + this.pos;
				}
			}
			this.lastsourceposlowered = this.pos;
			this.lastsourcecookedlowered = this.cookdata;
		}
		return this.cachedsourcefootprintlowered;
	}

	// Token: 0x0600098E RID: 2446 RVA: 0x00022D4C File Offset: 0x0002114C
	public Coord[] SourceFootprint()
	{
		if (this.type == EntType.island)
		{
			return this.IslandSourceFootprint();
		}
		int num = ((!this.Extended()) ? 0 : 1);
		int num2 = (int)(num + (int)Direction.West * (int)this.direction);
		if (this.lastsourcepos != this.pos || this.lastsourcecooked != num2)
		{
			if (num == 1)
			{
				if (this.cachedsourcefootprint == null || this.cachedsourcefootprint.Length != 2)
				{
					this.cachedsourcefootprint = new Coord[]
					{
						this.pos,
						this.pos + this.direction
					};
				}
				else
				{
					this.cachedsourcefootprint[0] = this.pos;
					this.cachedsourcefootprint[1] = this.pos + this.direction;
				}
			}
			else if (this.cachedsourcefootprint == null || this.cachedsourcefootprint.Length != 1)
			{
				this.cachedsourcefootprint = new Coord[] { this.pos };
			}
			else
			{
				this.cachedsourcefootprint[0] = this.pos;
			}
			this.lastsourcepos = this.pos;
			this.lastsourcecooked = num2;
		}
		return this.cachedsourcefootprint;
	}

	// Token: 0x0600098F RID: 2447 RVA: 0x00022EB8 File Offset: 0x000212B8
	public Coord[] SourceFootprintLower()
	{
		if (this.type == EntType.island)
		{
			return this.IslandSourceFootprintLower();
		}
		int num = ((!this.Extended()) ? 0 : 1);
		int num2 = (int)(num + (int)Direction.West * (int)this.direction);
		if (this.lastsourceposlowered != this.pos || this.lastsourcecookedlowered != num2)
		{
			if (num == 1)
			{
				if (this.cachedsourcefootprintlowered == null || this.cachedsourcefootprintlowered.Length != 2)
				{
					this.cachedsourcefootprintlowered = new Coord[]
					{
						this.pos + Direction.Down,
						this.pos + this.direction + Direction.Down
					};
				}
				else
				{
					this.cachedsourcefootprintlowered[0] = this.pos + Direction.Down;
					this.cachedsourcefootprintlowered[1] = this.pos + this.direction + Direction.Down;
				}
			}
			else if (this.cachedsourcefootprintlowered == null || this.cachedsourcefootprint.Length != 1)
			{
				this.cachedsourcefootprintlowered = new Coord[] { this.pos + Direction.Down };
			}
			else
			{
				this.cachedsourcefootprintlowered[0] = this.pos + Direction.Down;
			}
			this.lastsourceposlowered = this.pos;
			this.lastsourcecookedlowered = num2;
		}
		return this.cachedsourcefootprintlowered;
	}

	// Token: 0x06000990 RID: 2448 RVA: 0x0002304C File Offset: 0x0002144C
	public BoundingBox RoughOccupancyBounds()
	{
		int num = this.pos.x;
		int num2 = this.pos.y;
		int num3 = this.pos.z;
		int num4 = this.pos.x;
		int num5 = this.pos.y;
		int num6 = this.pos.z;
		if (this.type == EntType.island)
		{
			if (this.mask == null)
			{
				this.mask = this.gamestate.metagame.islandmasks[this.dat];
			}
			num += this.mask.offset.x;
			num2 += this.mask.offset.y;
			num3 += this.mask.offset.z;
			int num7 = this.mask.mask.Length;
			int num8 = ((num7 != 0) ? this.mask.mask[0].Length : 0);
			int num9 = ((num8 != 0) ? this.mask.mask[0][0].Length : 0);
			num4 += this.mask.offset.x + num7 - 1;
			num5 += this.mask.offset.y + num8 - 1;
			num6 += this.mask.offset.z + num9 - 1;
		}
		else if (this.Extended())
		{
			num--;
			num2--;
			num3--;
			num4++;
			num5++;
			num6++;
		}
		Movement movement = this.movement;
		BoundingBox boundingBox;
		if (movement == null || movement.mtype == Movement.MType.Surprise_Chasm)
		{
			boundingBox = new BoundingBox(new Coord(num, num2, num3), new Coord(num4, num5, num6), this.id);
		}
		else if (movement.translation)
		{
			switch (movement.direction)
			{
			case Direction.North:
				boundingBox = new BoundingBox(new Coord(num, num2 - 1, num3), new Coord(num4, num5, num6), this.id);
				goto IL_0359;
			case Direction.South:
				boundingBox = new BoundingBox(new Coord(num, num2, num3), new Coord(num4, num5 + 1, num6), this.id);
				goto IL_0359;
			case Direction.West:
				boundingBox = new BoundingBox(new Coord(num - 1, num2, num3), new Coord(num4, num5, num6), this.id);
				goto IL_0359;
			case Direction.East:
				boundingBox = new BoundingBox(new Coord(num, num2, num3), new Coord(num4 + 1, num5, num6), this.id);
				goto IL_0359;
			case Direction.None:
				boundingBox = new BoundingBox(new Coord(num, num2, num3), new Coord(num4, num5, num6), this.id);
				goto IL_0359;
			case Direction.Down:
				boundingBox = new BoundingBox(new Coord(num, num2, num3 - 1), new Coord(num4, num5, num6), this.id);
				goto IL_0359;
			case Direction.Up:
				boundingBox = new BoundingBox(new Coord(num, num2, num3), new Coord(num4, num5, num6 + 1), this.id);
				goto IL_0359;
			}
			Debug.LogError(movement.direction);
			boundingBox = new BoundingBox(new Coord(num, num2, num3), new Coord(num4, num5, num6), this.id);
			IL_0359:;
		}
		else
		{
			boundingBox = new BoundingBox(new Coord(num - 1, num2 - 1, num3 - 1), new Coord(num4 + 1, num5 + 1, num6 + 1), this.id);
		}
		return boundingBox;
	}

	// Token: 0x06000991 RID: 2449 RVA: 0x000233E4 File Offset: 0x000217E4
	public void CalcRoughOccupancyBounds()
	{
		int num = this.pos.x - 1;
		int num2 = this.pos.y - 1;
		int num3 = this.pos.z - 1;
		int num4 = this.pos.x + 1;
		int num5 = this.pos.y + 1;
		int num6 = this.pos.z + 1;
		if (this.type == EntType.island)
		{
			if (this.mask == null && (this.gamestate.metagame == null || !this.gamestate.metagame.islandmasks.TryGetValue(this.dat, out this.mask)))
			{
				this.mask = new IslandMask(new int[0][][], Coord.Zero);
			}
			num += this.mask.offset.x;
			num2 += this.mask.offset.y;
			num3 += this.mask.offset.z;
			int num7 = this.mask.mask.Length;
			int num8 = ((num7 != 0) ? this.mask.mask[0].Length : 0);
			int num9 = ((num8 != 0) ? this.mask.mask[0][0].Length : 0);
			num4 += this.mask.offset.x + num7 - 1;
			num5 += this.mask.offset.y + num8 - 1;
			num6 += this.mask.offset.z + num9 - 1;
		}
		else if (this.Extended())
		{
			num--;
			num2--;
			num3--;
			num4++;
			num5++;
			num6++;
		}
		this.oldbbox_rough = new BoundingBox(new Coord(num, num2, num3), new Coord(num4, num5, num6), this.id);
	}

	// Token: 0x06000992 RID: 2450 RVA: 0x000235CB File Offset: 0x000219CB
	public BoundingBox RoughOccupancyBounds_Wide()
	{
		return this.oldbbox_rough;
	}

	// Token: 0x06000993 RID: 2451 RVA: 0x000235D4 File Offset: 0x000219D4
	public void CalcIslandOccupancyMask()
	{
		if (this.islandoccupancymaskcalculated)
		{
			return;
		}
		if (this.gamestate.metagame == null)
		{
			return;
		}
		if (this.IslandOccupancyies_Dynamic_Enter != null)
		{
			return;
		}
		if (this._islandbordercaches == null)
		{
			this._islandbordercaches = new Coord[][]
			{
				new Coord[0],
				new Coord[0],
				new Coord[0],
				new Coord[0],
				new Coord[0],
				new Coord[0]
			};
		}
		if (Entity.IslandOccupanciesRepository.ContainsKey(this.dat))
		{
			this.IslandOccupancyies_Dynamic_Enter = Entity.IslandOccupanciesRepository[this.dat + "_Enter"];
			this.IslandOccupancyies_Dynamic_Exit = Entity.IslandOccupanciesRepository[this.dat + "_Exit"];
			this.IslandOccupancyies_Static = Entity.IslandOccupanciesRepository[this.dat];
			this.IslandOccupancyies_Dynamic_Enter_Completed = Entity.IslandOccupanciesRepository[this.dat + "_Enter_Completed"];
			this.IslandOccupancyies_Dynamic_Exit_Completed = Entity.IslandOccupanciesRepository[this.dat + "_Exit_Completed"];
			this.IslandOccupancyies_Static_Completed = Entity.IslandOccupanciesRepository[this.dat + "_Completed"];
			return;
		}
		if (!this.hiddenfootprintcalculated)
		{
			this.CalcHiddenFootprint();
		}
		this.IslandOccupancyies_Dynamic_Enter = new Coord[6][];
		this.IslandOccupancyies_Dynamic_Exit = new Coord[6][];
		this.IslandOccupancyies_Static = new Coord[6][];
		this.IslandOccupancyies_Dynamic_Enter_Completed = new Coord[6][];
		this.IslandOccupancyies_Dynamic_Exit_Completed = new Coord[6][];
		this.IslandOccupancyies_Static_Completed = new Coord[6][];
		Entity.CoordComparer coordComparer = new Entity.CoordComparer();
		for (int i = 0; i < 6; i++)
		{
			Direction direction;
			if (i < 4)
			{
				direction = (Direction)i;
			}
			else if (i == 4)
			{
				direction = Direction.Down;
			}
			else
			{
				direction = Direction.Up;
			}
			Direction direction2 = direction.Inverse();
			Entity.dynamic_fp_enter.Clear();
			Entity.dynamic_fp_exit.Clear();
			Entity.static_fp.Clear();
			foreach (Coord coord in this.hiddenfootprint)
			{
				bool flag = Array.BinarySearch<Coord>(this.hiddenfootprint, coord + direction2, coordComparer) >= 0;
				bool flag2 = Array.BinarySearch<Coord>(this.hiddenfootprint, coord + direction, coordComparer) >= 0;
				if (flag)
				{
					Entity.static_fp.Add(coord);
				}
				else
				{
					Entity.dynamic_fp_exit.Add(coord);
				}
				if (!flag2)
				{
					Entity.dynamic_fp_enter.Add(coord + direction);
				}
			}
			this.IslandOccupancyies_Dynamic_Enter[i] = Entity.dynamic_fp_enter.ToArray();
			this.IslandOccupancyies_Dynamic_Exit[i] = Entity.dynamic_fp_exit.ToArray();
			this.IslandOccupancyies_Static[i] = Entity.static_fp.ToArray();
			Entity.dynamic_fp_enter_completed.Clear();
			Entity.dynamic_fp_exit_completed.Clear();
			Entity.static_fp_completed.Clear();
			foreach (Coord coord2 in this.hiddenfootprint_completed)
			{
				bool flag3 = Array.BinarySearch<Coord>(this.hiddenfootprint_completed, coord2 + direction2, coordComparer) >= 0;
				bool flag4 = Array.BinarySearch<Coord>(this.hiddenfootprint_completed, coord2 + direction, coordComparer) >= 0;
				if (flag3)
				{
					Entity.static_fp_completed.Add(coord2);
				}
				else
				{
					Entity.dynamic_fp_exit_completed.Add(coord2);
				}
				if (!flag4)
				{
					Entity.dynamic_fp_enter_completed.Add(coord2 + direction);
				}
			}
			this.IslandOccupancyies_Dynamic_Enter_Completed[i] = Entity.dynamic_fp_enter_completed.ToArray();
			this.IslandOccupancyies_Dynamic_Exit_Completed[i] = Entity.dynamic_fp_exit_completed.ToArray();
			this.IslandOccupancyies_Static_Completed[i] = Entity.static_fp_completed.ToArray();
		}
		Entity.IslandOccupanciesRepository.Add(this.dat, this.IslandOccupancyies_Static);
		Entity.IslandOccupanciesRepository.Add(this.dat + "_Enter", this.IslandOccupancyies_Dynamic_Enter);
		Entity.IslandOccupanciesRepository.Add(this.dat + "_Exit", this.IslandOccupancyies_Dynamic_Exit);
		Entity.IslandOccupanciesRepository.Add(this.dat + "_Completed", this.IslandOccupancyies_Static_Completed);
		Entity.IslandOccupanciesRepository.Add(this.dat + "_Enter_Completed", this.IslandOccupancyies_Dynamic_Enter_Completed);
		Entity.IslandOccupanciesRepository.Add(this.dat + "_Exit_Completed", this.IslandOccupancyies_Dynamic_Exit_Completed);
		this.islandoccupancymaskcalculated = true;
	}

	// Token: 0x06000994 RID: 2452 RVA: 0x00023A6C File Offset: 0x00021E6C
	public bool TerraFirma()
	{
		return this.type != EntType.player && this.type != EntType.fork && this.type != EntType.sausage && !this.Decoration();
	}

	// Token: 0x06000995 RID: 2453 RVA: 0x00023AA0 File Offset: 0x00021EA0
	private Occupancy[] IslandTranslationOccupancy(Movement m)
	{
		if (!this.islandoccupancymaskcalculated)
		{
			this.CalcIslandOccupancyMask();
		}
		int num = (int)m.direction;
		if (num >= 4)
		{
			num -= 5;
		}
		Coord[] array;
		Coord[] array2;
		Coord[] array3;
		if (this.cookdata == 0)
		{
			array = this.IslandOccupancyies_Dynamic_Enter[num];
			array2 = this.IslandOccupancyies_Dynamic_Exit[num];
			array3 = this.IslandOccupancyies_Static[num];
		}
		else
		{
			array = this.IslandOccupancyies_Dynamic_Enter_Completed[num];
			array2 = this.IslandOccupancyies_Dynamic_Exit_Completed[num];
			array3 = this.IslandOccupancyies_Static_Completed[num];
		}
		Fraction fraction = 1 - m.remaining * m.speed;
		int num2 = array.Length + array2.Length + array3.Length;
		if (this.translationoccupancylist == null || this.translationoccupancylist.Length != num2)
		{
			this.translationoccupancylist = new Occupancy[num2];
		}
		for (int i = 0; i < array.Length; i++)
		{
			Coord coord = array[i] + this.pos;
			this.translationoccupancylist[i] = Entity.BuildOccupancy(coord, m.direction, true, fraction, m.speed);
		}
		int num3 = array.Length;
		for (int j = 0; j < array2.Length; j++)
		{
			Coord coord2 = array2[j] + this.pos;
			this.translationoccupancylist[j + num3] = Entity.BuildOccupancy(coord2, m.direction, false, fraction, m.speed);
		}
		num3 = array.Length + array2.Length;
		for (int k = 0; k < array3.Length; k++)
		{
			Coord coord3 = array3[k] + this.pos;
			this.translationoccupancylist[k + num3] = Entity.BuildOccupancy(coord3, Direction.None, true, 0, 0);
		}
		return this.translationoccupancylist;
	}

	// Token: 0x06000996 RID: 2454 RVA: 0x00023C88 File Offset: 0x00022088
	public static void BuildOccupancyCache()
	{
		if (Entity.occupancysupply == null)
		{
			Entity.occupancysupply = new Occupancy[10000];
			for (int i = 0; i < Entity.occupancysupply.Length; i++)
			{
				Entity.occupancysupply[i] = new Occupancy(Coord.Zero, Direction.None, false, default(Fraction), 0);
			}
		}
	}

	// Token: 0x06000997 RID: 2455 RVA: 0x00023CEC File Offset: 0x000220EC
	public static Occupancy BuildOccupancy(Coord _pos, Direction _dir, bool _entering, Fraction _position, int _speed)
	{
		Occupancy occupancy = Entity.occupancysupply[Entity.occupancysupplyindex];
		Entity.occupancysupplyindex = (Entity.occupancysupplyindex + 1) % 10000;
		occupancy.pos = _pos;
		occupancy.dir = _dir;
		occupancy.entering = _entering;
		occupancy.position = _position;
		occupancy.speed = _speed;
		return occupancy;
	}

	// Token: 0x06000998 RID: 2456 RVA: 0x00023D4C File Offset: 0x0002214C
	public Occupancy[] Occupancy()
	{
		if (this.movement != null && this.movement.movetype != Movement.MoveType.None)
		{
			int den = this.movement.remaining.den;
			int num = den - this.movement.remaining.num * this.movement.speed;
			Fraction fraction = new Fraction(num, den);
			if (this.movement.translation)
			{
				if (this.type == EntType.island)
				{
					return this.IslandTranslationOccupancy(this.movement);
				}
				Coord[] array = this.SourceFootprint();
				if (this._myoccupancy.Length != array.Length * 2)
				{
					this._myoccupancy = new Occupancy[array.Length * 2];
				}
				for (int i = 0; i < array.Length; i++)
				{
					Coord coord = array[i];
					this._myoccupancy[2 * i] = Entity.BuildOccupancy(coord, this.movement.direction, false, fraction, this.movement.speed);
					this._myoccupancy[2 * i + 1] = Entity.BuildOccupancy(coord + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
				}
				return this._myoccupancy;
			}
			else if (this.movement.rotation)
			{
				if (this.Extended())
				{
					if (this._myoccupancy.Length != 3)
					{
						this._myoccupancy = new Occupancy[3];
					}
					this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, Direction.None, false, fraction, this.movement.speed);
					Coord coord2 = this.pos + this.movement.to;
					Direction direction;
					if (this.movement.from.Ortho())
					{
						direction = DirectionUtil.ContinueRot(this.movement.from, this.movement.to);
					}
					else
					{
						direction = DirectionUtil.ContinueRot(this.movement.to, DirectionUtil.ContinueRot(this.movement.from, this.movement.to));
					}
					this._myoccupancy[1] = Entity.BuildOccupancy(this.pos + this.movement.from, direction, false, fraction, this.movement.speed);
					this._myoccupancy[2] = Entity.BuildOccupancy(coord2, direction, true, fraction, this.movement.speed);
					return this._myoccupancy;
				}
				if (this._myoccupancy.Length != 1)
				{
					this._myoccupancy = new Occupancy[1];
				}
				this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, Direction.None, true, 0, 0);
				return this._myoccupancy;
			}
			else
			{
				if (!this.Extended())
				{
					if (this._myoccupancy.Length != 1)
					{
						this._myoccupancy = new Occupancy[1];
					}
					this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, Direction.None, true, 0, 0);
					return this._myoccupancy;
				}
				if (this.movement.mtype == Movement.MType.TurnIn)
				{
					if (this.movement.from == this.movement.direction)
					{
						if (this._myoccupancy.Length != 3)
						{
							this._myoccupancy = new Occupancy[3];
						}
						this._myoccupancy[0] = Entity.BuildOccupancy(this.pos + this.movement.direction, Direction.None, true, 0, 0);
						this._myoccupancy[1] = Entity.BuildOccupancy(this.pos, this.movement.direction.Inverse(), false, fraction, this.movement.speed);
						this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + this.movement.direction + this.movement.to, this.movement.direction, true, fraction, this.movement.speed);
						return this._myoccupancy;
					}
					if (this.movement.from == this.movement.direction.Inverse())
					{
						Direction direction2 = DirectionUtil.ContinueRot(this.movement.from, this.movement.to);
						if (this._myoccupancy.Length != 4)
						{
							this._myoccupancy = new Occupancy[4];
						}
						this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, direction2, false, fraction, this.movement.speed);
						this._myoccupancy[1] = Entity.BuildOccupancy(this.pos + this.movement.from, this.movement.direction, false, fraction, this.movement.speed);
						this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + direction2, this.movement.direction, true, fraction, this.movement.speed);
						this._myoccupancy[3] = Entity.BuildOccupancy(this.pos + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
						return this._myoccupancy;
					}
					if (DirectionUtil.RotBetween(this.movement.from, this.movement.direction) == this.movement.to)
					{
						if (this._myoccupancy.Length != 4)
						{
							this._myoccupancy = new Occupancy[4];
						}
						this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, this.movement.direction, false, fraction, this.movement.speed);
						this._myoccupancy[1] = Entity.BuildOccupancy(this.pos + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
						this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + this.movement.to, this.movement.from, true, fraction, this.movement.speed + 1);
						this._myoccupancy[3] = Entity.BuildOccupancy(this.pos + this.movement.to + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
						return this._myoccupancy;
					}
					if (this._myoccupancy.Length != 3)
					{
						this._myoccupancy = new Occupancy[3];
					}
					this._myoccupancy[0] = Entity.BuildOccupancy(this.pos + this.direction, Direction.None, true, 0, 0);
					this._myoccupancy[1] = Entity.BuildOccupancy(this.pos, this.movement.direction, false, fraction, this.movement.speed);
					this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
					return this._myoccupancy;
				}
				else
				{
					if (this.movement.to == this.movement.direction)
					{
						Direction direction3 = DirectionUtil.ContinueRot(this.movement.to, this.movement.from).Inverse();
						if (this._myoccupancy.Length != 5)
						{
							this._myoccupancy = new Occupancy[5];
						}
						this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, this.movement.direction, false, fraction, this.movement.speed);
						this._myoccupancy[1] = Entity.BuildOccupancy(this.pos + this.movement.direction, direction3, true, fraction, this.movement.speed);
						this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + this.movement.direction + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
						this._myoccupancy[3] = Entity.BuildOccupancy(this.pos - direction3 + this.movement.direction, this.movement.direction, false, fraction, this.movement.speed);
						this._myoccupancy[4] = Entity.BuildOccupancy(this.pos - direction3 + this.movement.direction + this.movement.direction, direction3, false, fraction, this.movement.speed);
						return this._myoccupancy;
					}
					if (this.movement.to == this.movement.direction.Inverse())
					{
						if (this._myoccupancy.Length != 3)
						{
							this._myoccupancy = new Occupancy[3];
						}
						this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, Direction.None, true, 0, 0);
						this._myoccupancy[1] = Entity.BuildOccupancy(this.pos + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
						this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + this.movement.from, this.movement.direction, false, fraction, this.movement.speed);
						return this._myoccupancy;
					}
					if (DirectionUtil.ContinueRot(this.movement.to, this.movement.from) == this.movement.direction)
					{
						if (this._myoccupancy.Length != 3)
						{
							this._myoccupancy = new Occupancy[3];
						}
						this._myoccupancy[0] = Entity.BuildOccupancy(this.pos + this.direction, Direction.None, true, 0, 0);
						this._myoccupancy[1] = Entity.BuildOccupancy(this.pos, this.movement.direction, false, fraction, this.movement.speed);
						this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
						return this._myoccupancy;
					}
					if (this._myoccupancy.Length != 4)
					{
						this._myoccupancy = new Occupancy[4];
					}
					this._myoccupancy[0] = Entity.BuildOccupancy(this.pos, this.movement.direction, false, fraction, this.movement.speed);
					this._myoccupancy[1] = Entity.BuildOccupancy(this.pos + this.movement.to, this.movement.to, true, fraction, this.movement.speed + 1);
					this._myoccupancy[2] = Entity.BuildOccupancy(this.pos + this.movement.direction, this.movement.direction, true, fraction, this.movement.speed);
					this._myoccupancy[3] = Entity.BuildOccupancy(this.pos + this.movement.direction + this.movement.to, this.movement.direction, true, fraction, this.movement.speed);
					return this._myoccupancy;
				}
			}
		}
		else
		{
			if (this.direction == (Direction)this.occ_olddir && this.pos == this.occ_oldpos && this.occ_oldcookdat == this.cookdata)
			{
				return this.occ_oldocc;
			}
			Coord[] array2 = this.SourceFootprint();
			if (this.occ_oldocc == null || this.occ_oldocc.Length != array2.Length)
			{
				this.occ_oldocc = new Occupancy[array2.Length];
				for (int j = 0; j < array2.Length; j++)
				{
					this.occ_oldocc[j] = new Occupancy(array2[j], Direction.None, true, 0, 0);
				}
			}
			else
			{
				for (int k = 0; k < array2.Length; k++)
				{
					this.occ_oldocc[k].pos = array2[k];
				}
			}
			this.occ_olddir = (int)this.direction;
			this.occ_oldpos = this.pos;
			this.occ_oldcookdat = this.cookdata;
			return this.occ_oldocc;
		}
	}

	// Token: 0x06000999 RID: 2457 RVA: 0x00024AF8 File Offset: 0x00022EF8
	public static int DistanceSqFromPlayer(Entity a, Entity b)
	{
		if (a.type == EntType.island)
		{
			return Coord.DistanceSq(a.pos, b.pos);
		}
		Coord[] array = a.SourceFootprint();
		Coord[] array2 = b.SourceFootprint();
		int num = 100000;
		foreach (Coord coord in array)
		{
			foreach (Coord coord2 in array2)
			{
				int num2 = Coord.DistanceSq(coord, coord2);
				if (num2 < num)
				{
					num = num2;
				}
			}
		}
		return num;
	}

	// Token: 0x0600099A RID: 2458 RVA: 0x00024BA4 File Offset: 0x00022FA4
	public static float Distance(Entity a, Entity b)
	{
		if (a.type == EntType.island || b.type == EntType.island)
		{
			return Coord.Distance(a.pos, b.pos);
		}
		Coord[] array = a.SourceFootprint();
		Coord[] array2 = b.SourceFootprint();
		float num = 100000f;
		foreach (Coord coord in array)
		{
			foreach (Coord coord2 in array2)
			{
				float num2 = Coord.Distance(coord, coord2);
				if (num2 < num)
				{
					num = num2;
				}
			}
		}
		return num;
	}

	// Token: 0x0600099B RID: 2459 RVA: 0x00024C5C File Offset: 0x0002305C
	public List<Occupancy> MockOccupancy(Direction d, int speed)
	{
		if (this.movement != null)
		{
			Debug.LogError("EEK");
		}
		if (this.type == EntType.island)
		{
			Movement movement = Movement.Translation(null, d, 0, speed, Movement.MType.Backpedal, false);
			return this.IslandTranslationOccupancy(movement).ToList<Occupancy>();
		}
		Coord[] array = this.SourceFootprint();
		this.occupancybak.Clear();
		if (this.occupancybak.Capacity < array.Length * 2)
		{
			this.occupancybak.Capacity = array.Length * 2;
		}
		foreach (Coord coord in array)
		{
			Occupancy occupancy = Entity.BuildOccupancy(coord, d, false, Fraction.zero, speed);
			this.occupancybak.Add(occupancy);
			Occupancy occupancy2 = Entity.BuildOccupancy(coord + d, d, true, Fraction.zero, speed);
			this.occupancybak.Add(occupancy2);
		}
		return this.occupancybak;
	}

	// Token: 0x0600099C RID: 2460 RVA: 0x00024D48 File Offset: 0x00023148
	public bool CanMove(bool recurse = true, bool ignorefork = false)
	{
		return !this.Collides(recurse, ignorefork);
	}

	// Token: 0x0600099D RID: 2461 RVA: 0x00024D58 File Offset: 0x00023158
	public bool CouldMove(Direction dir, int speed, bool recurse = true)
	{
		if (this.movement != null)
		{
			return false;
		}
		List<Occupancy> list = this.MockOccupancy(dir, speed);
		BoundingBox boundingBox = this.RoughOccupancyBounds_Wide();
		this.gamestate.CalcBoxNeighbours(boundingBox, Entity._boxneighbours);
		foreach (HashSet<Entity> hashSet in Entity._boxneighbours)
		{
			foreach (Entity entity in hashSet)
			{
				if (entity != this && entity.stuckto != this.id)
				{
					BoundingBox boundingBox2 = entity.RoughOccupancyBounds();
					if (BoundingBox.Overlaps(boundingBox, boundingBox2))
					{
						if (entity.type == EntType.island && this.type == EntType.island && (this.movement == null || this.movement.direction.ParallelTo(Direction.Up)) && (entity.movement == null || entity.movement.direction.ParallelTo(Direction.Up)))
						{
							IslandMask islandMask = this.gamestate.metagame.islandmasks[this.dat];
							IslandMask islandMask2 = this.gamestate.metagame.islandmasks[entity.dat];
							bool[,] array = this.gamestate.metagame.projectioncompatibilities[this.dat][entity.dat];
							int num = this.pos.x + islandMask.offset.x - entity.pos.x - islandMask2.offset.x;
							int num2 = this.pos.y + islandMask.offset.y - entity.pos.y - islandMask2.offset.y;
							int num3 = islandMask.mask.Length;
							int num4 = ((num3 != 0) ? islandMask.mask[0].Length : 0);
							int num5 = num + num3 - 1;
							int num6 = num2 + num4 - 1;
							if (num5 < 0 || num6 < 0 || num5 >= array.GetLength(0) || num6 >= array.GetLength(1) || array[num5, num6])
							{
								continue;
							}
						}
						BoundingBox boundingBox3 = boundingBox.Intersect(boundingBox2);
						Occupancy[] array2 = entity.Occupancy();
						this.occupancies2.Clear();
						foreach (Occupancy occupancy in array2)
						{
							if (boundingBox3.Overlaps(occupancy.pos))
							{
								this.occupancies2.Add(occupancy);
							}
						}
						foreach (Occupancy occupancy2 in list)
						{
							if (boundingBox3.Overlaps(occupancy2.pos))
							{
								foreach (Occupancy occupancy3 in this.occupancies2)
								{
									if (occupancy2.Overlaps(occupancy3))
									{
										return false;
									}
								}
							}
						}
						continue;
					}
				}
			}
		}
		if (recurse && this.stuckto >= 0)
		{
			Entity entity2 = this.gamestate.FromIDDynamic(this.stuckto);
			return entity2.movement == null && entity2.CouldMove(dir, speed, false);
		}
		return true;
	}

	// Token: 0x0600099E RID: 2462 RVA: 0x0002511C File Offset: 0x0002351C
	public bool AboutToMove()
	{
		return this.movement != null && this.movement.Starting();
	}

	// Token: 0x0600099F RID: 2463 RVA: 0x00025138 File Offset: 0x00023538
	public void AppendIslandBBQPositions(List<Coord> positions)
	{
		if (this.mask == null)
		{
			this.mask = this.gamestate.metagame.islandmasks[this.dat];
		}
		int num = this.mask.mask.Length;
		int num2 = ((num != 0) ? this.mask.mask[0].Length : 0);
		int num3 = ((num2 != 0) ? this.mask.mask[0][0].Length : 0);
		for (int i = 0; i < num; i++)
		{
			for (int j = 0; j < num2; j++)
			{
				for (int k = 0; k < num3; k++)
				{
					if (this.mask.mask[i][j][k] == 2 || this.mask.mask[i][j][k] == 20)
					{
						positions.Add(this.pos + this.mask.offset + new Coord(i, j, k));
					}
				}
			}
		}
	}

	// Token: 0x060009A0 RID: 2464 RVA: 0x00025258 File Offset: 0x00023658
	public string QuickName()
	{
		if (this.type == EntType.player)
		{
			return "Player";
		}
		if (this.type == EntType.island)
		{
			return this.dat;
		}
		if (this.type == EntType.sausage)
		{
			return "Sausage" + this.direction.ToString() + this.pos.ToString();
		}
		return this.type.ToString() + this.id.ToStringFast();
	}

	// Token: 0x060009A1 RID: 2465 RVA: 0x000252E4 File Offset: 0x000236E4
	public bool Collides(bool recurse = true, bool ignorefork = false)
	{
		if (this.type == EntType.fork && this.stuckto == this.gamestate.player.id)
		{
			return false;
		}
		if (this.movement == null)
		{
			Debug.LogError("EEK4");
		}
		Occupancy[] array = null;
		BoundingBox boundingBox = this.RoughOccupancyBounds();
		this.gamestate.CalcBoxNeighbours(boundingBox, Entity._boxneighbours);
		for (int i = 0; i < Entity._boxneighbours.Count; i++)
		{
			HashSet<Entity> hashSet = Entity._boxneighbours[i];
			foreach (Entity entity in hashSet)
			{
				if (entity != this && entity.stuckto != this.id)
				{
					if (!ignorefork || entity.type != EntType.fork)
					{
						if (!entity.Decoration())
						{
							if (entity.movement == null || !entity.movement.translation || !this.movement.translation || entity.movement.direction != this.movement.direction || entity.movement.speed != this.movement.speed)
							{
								BoundingBox boundingBox2 = entity.RoughOccupancyBounds();
								if ((entity.type != EntType.island && this.type != EntType.island) || BoundingBox.Overlaps(boundingBox, boundingBox2))
								{
									if (entity.type == EntType.island && this.type == EntType.island && (this.movement == null || this.movement.direction.ParallelTo(Direction.Up)) && (entity.movement == null || entity.movement.direction.ParallelTo(Direction.Up)))
									{
										IslandMask islandMask = this.gamestate.metagame.islandmasks[this.dat];
										IslandMask islandMask2 = this.gamestate.metagame.islandmasks[entity.dat];
										bool[,] array2 = this.gamestate.metagame.projectioncompatibilities[this.dat][entity.dat];
										int num = this.pos.x + islandMask.offset.x - entity.pos.x - islandMask2.offset.x;
										int num2 = this.pos.y + islandMask.offset.y - entity.pos.y - islandMask2.offset.y;
										int num3 = islandMask.mask.Length;
										int num4 = ((num3 != 0) ? islandMask.mask[0].Length : 0);
										int num5 = num + num3 - 1;
										int num6 = num2 + num4 - 1;
										if (num5 < 0 || num6 < 0 || num5 >= array2.GetLength(0) || num6 >= array2.GetLength(1) || array2[num5, num6])
										{
											continue;
										}
									}
									BoundingBox boundingBox3 = boundingBox.Intersect(boundingBox2);
									Occupancy[] array3 = entity.Occupancy();
									this.occupancies2.Clear();
									foreach (Occupancy occupancy in array3)
									{
										if (boundingBox3.Overlaps(occupancy.pos))
										{
											this.occupancies2.Add(occupancy);
										}
									}
									if (array == null)
									{
										array = this.Occupancy();
									}
									foreach (Occupancy occupancy2 in array)
									{
										if (boundingBox3.Overlaps(occupancy2.pos))
										{
											foreach (Occupancy occupancy3 in this.occupancies2)
											{
												if (occupancy2.Overlaps(occupancy3))
												{
													return true;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if (recurse && this.stuckto >= 0)
		{
			Entity entity2 = this.gamestate.FromIDDynamic(this.stuckto);
			if (entity2.movement != null)
			{
				return entity2.Collides(false, false);
			}
		}
		return false;
	}

	// Token: 0x170003CC RID: 972
	// (get) Token: 0x060009A2 RID: 2466 RVA: 0x00025748 File Offset: 0x00023B48
	public bool moving
	{
		get
		{
			return this.movement != null && this.movement.mtype != Movement.MType.Fixed;
		}
	}

	// Token: 0x060009A3 RID: 2467 RVA: 0x0002576C File Offset: 0x00023B6C
	private Coord[] IslandBorder(Direction d, bool recursive = true)
	{
		if (!this.islandoccupancymaskcalculated)
		{
			this.CalcIslandOccupancyMask();
		}
		int num = (int)d;
		if (num >= 4)
		{
			num -= 5;
		}
		Coord[] array = this._islandbordercaches[num];
		if (this.cookdata == 0)
		{
			int num2 = this.IslandOccupancyies_Dynamic_Enter[num].Length;
			if (array.Length != num2)
			{
				array = new Coord[this.IslandOccupancyies_Dynamic_Enter[num].Length];
				this._islandbordercaches[num] = array;
			}
			Coord[] array2 = this.IslandOccupancyies_Dynamic_Enter[num];
			for (int i = 0; i < num2; i++)
			{
				array[i] = array2[i] + this.pos;
			}
			return array;
		}
		int num3 = this.IslandOccupancyies_Dynamic_Enter_Completed[num].Length;
		if (array.Length != num3)
		{
			array = new Coord[this.IslandOccupancyies_Dynamic_Enter_Completed[num].Length];
			this._islandbordercaches[num] = array;
		}
		Coord[] array3 = this.IslandOccupancyies_Dynamic_Enter_Completed[num];
		for (int j = 0; j < num3; j++)
		{
			array[j] = array3[j] + this.pos;
		}
		return array;
	}

	// Token: 0x060009A4 RID: 2468 RVA: 0x00025898 File Offset: 0x00023C98
	public Coord[] Border(Direction d, bool recursive = true)
	{
		if (d.Diagonal())
		{
			Debug.LogError("eep");
		}
		if (this.type == EntType.island)
		{
			return this.IslandBorder(d, recursive);
		}
		if (!this.Extended())
		{
			if (this._border.Length != 1)
			{
				this._border = new Coord[] { this.pos + d };
			}
			else
			{
				this._border[0] = this.pos + d;
			}
		}
		else if (this.direction == d)
		{
			if (this._border.Length != 1)
			{
				this._border = new Coord[] { this.pos + 2 * d.ToCoord() };
			}
			else
			{
				this._border[0] = this.pos + 2 * d.ToCoord();
			}
		}
		else if (this.direction == d.Inverse())
		{
			if (this._border.Length != 1)
			{
				this._border = new Coord[] { this.pos + d };
			}
			else
			{
				this._border[0] = this.pos + d;
			}
		}
		else if (this._border.Length != 2)
		{
			this._border = new Coord[]
			{
				this.pos + d,
				this.pos + this.direction + d
			};
		}
		else
		{
			this._border[0] = this.pos + d;
			this._border[1] = this.pos + this.direction + d;
		}
		if (recursive && this.Laden() && this.LadenTarget().Extended())
		{
			Coord[] array = this.LadenTarget().Border(d, false);
			Coord[] array2 = new Coord[this._border.Length + array.Length];
			this._border.CopyTo(array2, 0);
			array.CopyTo(array2, this._border.Length);
			return array2;
		}
		return this._border;
	}

	// Token: 0x060009A5 RID: 2469 RVA: 0x00025B1C File Offset: 0x00023F1C
	public bool Laden()
	{
		return this.stuckto >= 0;
	}

	// Token: 0x060009A6 RID: 2470 RVA: 0x00025B2A File Offset: 0x00023F2A
	public Entity LadenTarget()
	{
		return this.gamestate.LadenTarget(this);
	}

	// Token: 0x04000565 RID: 1381
	public Coord pos;

	// Token: 0x04000566 RID: 1382
	public EntType type;

	// Token: 0x04000567 RID: 1383
	public int id;

	// Token: 0x04000568 RID: 1384
	public int tilenum;

	// Token: 0x04000569 RID: 1385
	public int tileset;

	// Token: 0x0400056A RID: 1386
	public Direction direction = Direction.None;

	// Token: 0x0400056B RID: 1387
	public string dat;

	// Token: 0x0400056C RID: 1388
	public int stuckto;

	// Token: 0x0400056D RID: 1389
	public int rot;

	// Token: 0x0400056E RID: 1390
	public int cookdata;

	// Token: 0x0400056F RID: 1391
	public Direction turndir = Direction.None;

	// Token: 0x04000570 RID: 1392
	public int pivot;

	// Token: 0x04000571 RID: 1393
	public GameState gamestate;

	// Token: 0x04000572 RID: 1394
	private EntitySkeleton _lastskel;

	// Token: 0x04000573 RID: 1395
	public Movement movement;

	// Token: 0x04000574 RID: 1396
	public Movement _movement;

	// Token: 0x04000575 RID: 1397
	private static List<int> _EntDatIndices = new List<int>(16);

	// Token: 0x04000576 RID: 1398
	private IslandMask mask;

	// Token: 0x04000577 RID: 1399
	private bool hiddenfootprintcalculated;

	// Token: 0x04000578 RID: 1400
	private Coord[] hiddenfootprint;

	// Token: 0x04000579 RID: 1401
	private Coord[] hiddenfootprintlower;

	// Token: 0x0400057A RID: 1402
	private Coord[] hiddenfootprint_completed;

	// Token: 0x0400057B RID: 1403
	private Coord[] hiddenfootprintlower_completed;

	// Token: 0x0400057C RID: 1404
	private Coord[] cachedsourcefootprint;

	// Token: 0x0400057D RID: 1405
	private Coord[] cachedsourcefootprintlowered;

	// Token: 0x0400057E RID: 1406
	private int lastsourcecooked = -100;

	// Token: 0x0400057F RID: 1407
	private int lastsourcecookedlowered = -100;

	// Token: 0x04000580 RID: 1408
	private Coord lastsourcepos = Coord.Invalid;

	// Token: 0x04000581 RID: 1409
	private Coord lastsourceposlowered = Coord.Invalid;

	// Token: 0x04000582 RID: 1410
	private BoundingBox oldbbox_rough;

	// Token: 0x04000583 RID: 1411
	private bool islandoccupancymaskcalculated;

	// Token: 0x04000584 RID: 1412
	private static StringDictionary<Coord[][]> IslandOccupanciesRepository = new StringDictionary<Coord[][]>();

	// Token: 0x04000585 RID: 1413
	private Coord[][] IslandOccupancyies_Dynamic_Enter;

	// Token: 0x04000586 RID: 1414
	private Coord[][] IslandOccupancyies_Dynamic_Exit;

	// Token: 0x04000587 RID: 1415
	private Coord[][] IslandOccupancyies_Static;

	// Token: 0x04000588 RID: 1416
	private Coord[][] IslandOccupancyies_Dynamic_Enter_Completed;

	// Token: 0x04000589 RID: 1417
	private Coord[][] IslandOccupancyies_Dynamic_Exit_Completed;

	// Token: 0x0400058A RID: 1418
	private Coord[][] IslandOccupancyies_Static_Completed;

	// Token: 0x0400058B RID: 1419
	private static List<Coord> dynamic_fp_enter = new List<Coord>();

	// Token: 0x0400058C RID: 1420
	private static List<Coord> dynamic_fp_exit = new List<Coord>();

	// Token: 0x0400058D RID: 1421
	private static List<Coord> static_fp = new List<Coord>();

	// Token: 0x0400058E RID: 1422
	private static List<Coord> dynamic_fp_enter_completed = new List<Coord>();

	// Token: 0x0400058F RID: 1423
	private static List<Coord> dynamic_fp_exit_completed = new List<Coord>();

	// Token: 0x04000590 RID: 1424
	private static List<Coord> static_fp_completed = new List<Coord>();

	// Token: 0x04000591 RID: 1425
	private Occupancy[] translationoccupancylist;

	// Token: 0x04000592 RID: 1426
	private Coord occ_oldpos;

	// Token: 0x04000593 RID: 1427
	private int occ_olddir = -1;

	// Token: 0x04000594 RID: 1428
	private int occ_oldcookdat = -100;

	// Token: 0x04000595 RID: 1429
	private Occupancy[] occ_oldocc;

	// Token: 0x04000596 RID: 1430
	private static Occupancy[] occupancysupply;

	// Token: 0x04000597 RID: 1431
	private static int occupancysupplyindex = 0;

	// Token: 0x04000598 RID: 1432
	private Occupancy[] _myoccupancy = new Occupancy[0];

	// Token: 0x04000599 RID: 1433
	private List<Occupancy> occupancybak = new List<Occupancy>();

	// Token: 0x0400059A RID: 1434
	private static List<HashSet<Entity>> _boxneighbours = new List<HashSet<Entity>>(4);

	// Token: 0x0400059B RID: 1435
	public List<Occupancy> occupancies2 = new List<Occupancy>();

	// Token: 0x0400059C RID: 1436
	private Coord[][] _islandbordercaches;

	// Token: 0x0400059D RID: 1437
	private Coord[] _border = new Coord[0];

	// Token: 0x0200009C RID: 156
	public class EntDat
	{
		// Token: 0x060009A8 RID: 2472 RVA: 0x00025BA8 File Offset: 0x00023FA8
		public EntDat(int[] arr, string dat)
		{
			this.arr = arr;
			this.dat = dat;
		}

		// Token: 0x0400059E RID: 1438
		public readonly int[] arr;

		// Token: 0x0400059F RID: 1439
		public readonly string dat;
	}

	// Token: 0x0200009D RID: 157
	public class CoordComparer : IComparer<Coord>
	{
		// Token: 0x060009AA RID: 2474 RVA: 0x00025BC8 File Offset: 0x00023FC8
		public int Compare(Coord a, Coord b)
		{
			if (a.x < b.x)
			{
				return -1;
			}
			if (a.x > b.x)
			{
				return 1;
			}
			if (a.y < b.y)
			{
				return -1;
			}
			if (a.y > b.y)
			{
				return 1;
			}
			if (a.z < b.z)
			{
				return -1;
			}
			if (a.z > b.z)
			{
				return 1;
			}
			return 0;
		}
	}
}
