using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

// Token: 0x020000A1 RID: 161
public class GameState
{
	// Token: 0x060009BC RID: 2492 RVA: 0x00026200 File Offset: 0x00024600
	public GameState()
	{
		this.BuildStaticCaches();
	}

	// Token: 0x060009BD RID: 2493 RVA: 0x0002639C File Offset: 0x0002479C
	public GameState(MetaGameState _metagame)
	{
		this.BuildStaticCaches();
		this.metagame = _metagame;
		if (_metagame != null)
		{
			this.metagame.gamestate = this;
		}
		this.entities = new List<Entity>();
		this.dynamicentities = new List<Entity>();
	}

	// Token: 0x060009BE RID: 2494 RVA: 0x00026568 File Offset: 0x00024968
	private GameState(string s, MetaGameState _metagame, bool precalc = true)
	{
		this.BuildStaticCaches();
		this.LoadDat(s, _metagame, precalc);
	}

	// Token: 0x060009BF RID: 2495 RVA: 0x0002670C File Offset: 0x00024B0C
	private void CheckOnLevelExit()
	{
		if (this.overworld || !this.Won() || this.metagame == null || this.LevelCompleted(this.pushtargetlevel) || !this.player.Extended())
		{
			return;
		}
		if (this.player.pos == this.exitPos && this.player.direction == this.exitDir && this.exitUp)
		{
			this.SubworldLeave();
		}
	}

	// Token: 0x060009C0 RID: 2496 RVA: 0x000267A0 File Offset: 0x00024BA0
	public Entity GetIsland(string islandname)
	{
		return this.entities.First<Entity>((Entity e) => e.type == EntType.island && e.dat == islandname);
	}

	// Token: 0x060009C1 RID: 2497 RVA: 0x000267D4 File Offset: 0x00024BD4
	private void CheckOverworldGhosts()
	{
		if (this.Lost().Length > 0)
		{
			return;
		}
		if (!this.overworld || this.metagame == null)
		{
			return;
		}
		Coord pos = this.player.pos;
		Direction direction = this.player.direction;
		foreach (string text in this.metagame.playerpositions.Keys)
		{
			KeyValuePair<Coord, Direction> keyValuePair = this.metagame.playerpositions[text];
			Coord key = keyValuePair.Key;
			Direction value = keyValuePair.Value;
			Entity entity = this.islandindex[text];
			if (pos == key + entity.pos)
			{
				if (value == direction && this.fork == null && !this.LevelCompleted(text))
				{
					bool flag = false;
					foreach (Entity entity2 in this.dynamicentities)
					{
						if (entity2.type == EntType.sausage && entity2.cookdata != 0)
						{
							flag = true;
							break;
						}
					}
					if (!flag)
					{
						this.SubworldTransition(text);
					}
				}
				break;
			}
		}
	}

	// Token: 0x060009C2 RID: 2498 RVA: 0x00026924 File Offset: 0x00024D24
	private void GetExitPos(string levelname, out Coord pos, out Direction dir)
	{
		foreach (string text in this.metagame.playerpositions.Keys)
		{
			if (!(text != levelname))
			{
				KeyValuePair<Coord, Direction> keyValuePair = this.metagame.playerpositions[text];
				Coord key = keyValuePair.Key;
				foreach (Entity entity in this.entities)
				{
					if (entity.type == EntType.island && entity.dat == this.pushtargetlevel)
					{
						Entity entity2 = entity;
						pos = key + entity2.pos;
						dir = keyValuePair.Value;
						return;
					}
				}
			}
		}
		Debug.LogError("EEP no start found for " + levelname);
		pos = Coord.Invalid;
		dir = Direction.None;
	}

	// Token: 0x060009C3 RID: 2499 RVA: 0x00026A44 File Offset: 0x00024E44
	private void SubworldTransition(string levelname)
	{
		Entity entity = this.islandindex[levelname];
		entity.cookdata = 1;
		this.overworld = false;
		this.pushtargetlevel = levelname;
		if (LoaderSaver._loadedLevelName.Contains("WorldExplore"))
		{
			this.pushestotry = 22;
		}
		else
		{
			this.pushestotry = 20;
		}
		this.SpawnSubworldSausages();
		this.startedfalling = true;
		this.TryLowerAll();
		this.GetExitPos(this.pushtargetlevel, out this.exitPos, out this.exitDir);
		this.exitUp = true;
		this.exitAttachment = null;
		Entity entity2 = this.EntAt(this.exitPos + Direction.Down, false, false);
		if (entity2.type == EntType.sausage)
		{
			this.exitAttachment = entity2;
		}
	}

	// Token: 0x060009C4 RID: 2500 RVA: 0x00026B00 File Offset: 0x00024F00
	public void SubworldLeave()
	{
		this.DespawnSubworldSausages();
		this.overworld = true;
		this.CompleteLevel(this.pushtargetlevel);
		GameState.lastpushed = this.pushtargetlevel;
		this.pushtargetlevel = string.Empty;
		if (LoaderSaver._loadedLevelName.Contains("WorldExplore"))
		{
			this.pushestotry = -22;
		}
		else
		{
			this.pushestotry = -20;
		}
		this.startedrising = true;
		this.TryRaiseAll();
	}

	// Token: 0x060009C5 RID: 2501 RVA: 0x00026B74 File Offset: 0x00024F74
	private void DespawnSubworldSausages()
	{
		for (int i = this.dynamicentities.Count - 1; i >= 0; i--)
		{
			Entity entity = this.dynamicentities[i];
			if (entity.type == EntType.sausage)
			{
				if (entity.dat.Length == 0 || entity.dat[0] == 'M')
				{
					this.sausageexplosions.Add(entity.pos);
					this.sausagedirs.Add(entity.direction);
					this.RemoveEntity(entity);
					this.sausagescooked++;
				}
				else if (entity.dat.Length > 0 && entity.dat[0] == 'S')
				{
					entity.dat = "M" + entity.dat.Substring(1);
				}
			}
		}
	}

	// Token: 0x060009C6 RID: 2502 RVA: 0x00026C57 File Offset: 0x00025057
	public bool SausageIssued(string shrinename)
	{
		return this.worldsausagesissued.Contains(shrinename);
	}

	// Token: 0x060009C7 RID: 2503 RVA: 0x00026C65 File Offset: 0x00025065
	public void SetSausageIssued(string shrinename)
	{
		this.levelcompleted.AddUnique(shrinename);
		this.worldsausagesissued.AddUnique(shrinename);
	}

	// Token: 0x060009C8 RID: 2504 RVA: 0x00026C81 File Offset: 0x00025081
	public bool LevelCompleted(string levelname)
	{
		return this.levelcompleted.Contains(levelname);
	}

	// Token: 0x060009C9 RID: 2505 RVA: 0x00026C90 File Offset: 0x00025090
	public void CompleteLevel(string levelname)
	{
		string text = levelname.Split(new string[] { "__" }, StringSplitOptions.None)[0];
		Debug.Log("completing" + levelname);
		this.levelcompleted.AddUnique(levelname);
		this.levelcompleted.AddUnique(text);
	}

	// Token: 0x060009CA RID: 2506 RVA: 0x00026CDF File Offset: 0x000250DF
	public void ClearSave()
	{
		this.levelcompleted.Clear();
		this.worldsausagesissued.Clear();
	}

	// Token: 0x060009CB RID: 2507 RVA: 0x00026CF8 File Offset: 0x000250F8
	public bool ShouldIssueSausage(string shrinename)
	{
		if (this.metagame == null)
		{
			return false;
		}
		if (this.SausageIssued(shrinename))
		{
			return false;
		}
		if (!this.metagame.IsShrine(shrinename))
		{
			return false;
		}
		foreach (string text in this.metagame.templedat[shrinename])
		{
			if (!this.LevelCompleted(text))
			{
				return false;
			}
		}
		return true;
	}

	// Token: 0x060009CC RID: 2508 RVA: 0x00026D9C File Offset: 0x0002519C
	private void SpawnSubworldSausages()
	{
		Entity entity = this.islandindex[this.pushtargetlevel];
		List<KeyValuePair<Coord, Direction>> list = this.metagame.sausagepositions[this.pushtargetlevel];
		foreach (KeyValuePair<Coord, Direction> keyValuePair in list)
		{
			this.AddEntity(new Entity(this)
			{
				type = EntType.sausage,
				direction = keyValuePair.Value,
				pos = keyValuePair.Key + entity.pos
			}, false);
		}
	}

	// Token: 0x060009CD RID: 2509 RVA: 0x00026E58 File Offset: 0x00025258
	public List<Entity> IssueWorldSausages()
	{
		List<Entity> list = new List<Entity>();
		foreach (string text in this.metagame.sausagepositions.Keys)
		{
			if (this.metagame.IsShrine(text))
			{
				Entity entity = this.islandindex[text];
				if (this.ShouldIssueSausage(text))
				{
					List<KeyValuePair<Coord, Direction>> list2 = this.metagame.sausagepositions[text];
					foreach (KeyValuePair<Coord, Direction> keyValuePair in list2)
					{
						Entity entity2 = new Entity(this);
						entity2.tileset = 1;
						entity2.type = EntType.sausage;
						entity2.pos = keyValuePair.Key + entity.pos;
						entity2.direction = keyValuePair.Value;
						this.AddEntity(entity2, false);
						list.Add(entity2);
						this.worldsausagespawns.Add(entity2.pos);
						this.worldsausagespawndirs.Add(entity2.direction);
						this.SetSausageIssued(text);
						GameState.shouldredrawcoffins = entity2.pos;
					}
					entity.cookdata = 1;
				}
			}
		}
		return list;
	}

	// Token: 0x060009CE RID: 2510 RVA: 0x00026FF0 File Offset: 0x000253F0
	public void CheckGameWon()
	{
		if (!this.overworld)
		{
			this.won = false;
			this.returning = false;
			return;
		}
		this.returning = true;
		this.won = true;
		foreach (Entity entity in this.dynamicentities)
		{
			if (entity.type == EntType.sausage)
			{
				int num = entity.cookdata;
				for (int i = 0; i < 4; i++)
				{
					int num2 = num % 4;
					if (num2 == 0 || num2 == 3)
					{
						this.won = false;
					}
					num /= 4;
				}
			}
		}
		foreach (string text in this.metagame.sausagepositions.Keys)
		{
			if (!this.SausageIssued(text) && this.metagame.IsShrine(text))
			{
				this.won = false;
				this.returning = false;
			}
		}
		if (this.won)
		{
			this.haveevercookedall = true;
		}
	}

	// Token: 0x060009CF RID: 2511 RVA: 0x00027128 File Offset: 0x00025528
	public IslandMask LevelMask(Coord _offset)
	{
		Entity[] array = this.entities.Where<Entity>((Entity e) => e.type.Static()).ToArray<Entity>();
		if (array.Length == 0)
		{
			return new IslandMask(new int[0][][], Coord.Zero);
		}
		Entity[] array2 = this.entities.Where<Entity>((Entity e) => e.type.Static() || e.type == EntType.sausage).ToArray<Entity>();
		IEnumerable<Entity> enumerable = this.entities.Where<Entity>((Entity e) => e.type == EntType.sausage);
		int num = array2.Min<Entity>((Entity e) => e.pos.x);
		int num2 = array2.Min<Entity>((Entity e) => e.pos.y);
		int num3 = array2.Min<Entity>((Entity e) => e.pos.z);
		int num4 = array2.Max<Entity>((Entity e) => e.pos.x);
		int num5 = array2.Max<Entity>((Entity e) => e.pos.y);
		int num6 = array2.Max<Entity>((Entity e) => e.pos.z);
		if (enumerable.Any<Entity>((Entity s) => s.direction == Direction.East))
		{
			num4 = Mathf.Max(num4, enumerable.Where<Entity>((Entity s) => s.direction == Direction.East).Max<Entity>((Entity s) => s.pos.x) + 1);
		}
		if (enumerable.Any<Entity>((Entity s) => s.direction == Direction.West))
		{
			num = Mathf.Min(num, enumerable.Where<Entity>((Entity s) => s.direction == Direction.West).Min<Entity>((Entity s) => s.pos.x) - 1);
		}
		if (enumerable.Any<Entity>((Entity s) => s.direction == Direction.South))
		{
			num5 = Mathf.Max(num5, enumerable.Where<Entity>((Entity s) => s.direction == Direction.South).Max<Entity>((Entity s) => s.pos.y) + 1);
		}
		if (enumerable.Any<Entity>((Entity s) => s.direction == Direction.North))
		{
			num2 = Mathf.Min(num2, enumerable.Where<Entity>((Entity s) => s.direction == Direction.North).Min<Entity>((Entity s) => s.pos.y) - 1);
		}
		int num7 = num4 - num + 1;
		int num8 = num5 - num2 + 1;
		int num9 = num6 - num3 + 1;
		Coord coord = new Coord(num, num2, num3);
		int[][][] array3 = new int[num7][][];
		for (int i = 0; i < num7; i++)
		{
			array3[i] = new int[num8][];
			for (int j = 0; j < num8; j++)
			{
				array3[i][j] = new int[num9];
			}
		}
		Entity[] array4 = array;
		int k = 0;
		while (k < array4.Length)
		{
			Entity entity = array4[k];
			int num10 = 1;
			switch (entity.type)
			{
			case EntType.ground:
				if (entity.Solid())
				{
					if (entity.tilenum == 6 && (entity.tileset == 6 || entity.tileset == 1))
					{
						int num11 = this.TreeColor(entity.pos);
						if (num11 > 0)
						{
							num10 = 7 + num11;
						}
					}
					else if (entity.tileset == 12 && entity.tilenum == 9)
					{
						num10 = (int)(9 + entity.direction);
					}
					else
					{
						num10 = entity.FootprintType(this.tileset);
					}
				}
				else
				{
					num10 = -10 - entity.DecorationType();
				}
				break;
			case EntType.bbq:
				if (entity.direction == Direction.North || entity.direction == Direction.South)
				{
					num10 = 20;
				}
				else
				{
					num10 = 2;
				}
				break;
			case EntType.ladder:
				num10 = (int)(3 + entity.direction);
				break;
			}
			IL_04F5:
			array3[entity.pos.x - num][entity.pos.y - num2][entity.pos.z - num3] = num10;
			k++;
			continue;
			goto IL_04F5;
		}
		foreach (Entity entity2 in enumerable)
		{
			Coord pos = entity2.pos;
			Coord coord2 = entity2.pos + entity2.direction;
			array3[pos.x - num][pos.y - num2][pos.z - num3] = -1;
			array3[coord2.x - num][coord2.y - num2][coord2.z - num3] = -1;
		}
		coord -= _offset;
		return new IslandMask(array3, coord);
	}

	// Token: 0x060009D0 RID: 2512 RVA: 0x00027708 File Offset: 0x00025B08
	public int TreeColor(Coord pos)
	{
		Entity entity = this.StaticEntAt(pos + 3 * Coord.Up);
		if (entity != null)
		{
			if (entity.tileset == 6 && entity.tilenum == 2)
			{
				return 1;
			}
			if (entity.tileset == 6 && entity.tilenum == 4)
			{
				return 2;
			}
		}
		return 0;
	}

	// Token: 0x060009D1 RID: 2513 RVA: 0x00027768 File Offset: 0x00025B68
	public bool HasStaticEntAt(Coord c)
	{
		Entity entity = this.StaticEntAt(c);
		return entity != null && entity.Solid();
	}

	// Token: 0x060009D2 RID: 2514 RVA: 0x00027794 File Offset: 0x00025B94
	public Entity StaticEntAt(Coord c)
	{
		if (c.x < this.smapmin.x || c.y < this.smapmin.y || c.z < this.smapmin.z || c.x > this.smapmax.x || c.y > this.smapmax.y || c.z > this.smapmax.z)
		{
			return null;
		}
		return this.staticmap[c.x - this.smapmin.x, c.y - this.smapmin.y, c.z - this.smapmin.z];
	}

	// Token: 0x060009D3 RID: 2515 RVA: 0x00027874 File Offset: 0x00025C74
	public List<Entity> StaticEntsAt(Coord[] coords)
	{
		List<Entity> list = new List<Entity>();
		foreach (Coord coord in coords)
		{
			Entity entity = this.StaticEntAt(coord);
			if (entity != null)
			{
				list.Add(entity);
			}
		}
		return list;
	}

	// Token: 0x060009D4 RID: 2516 RVA: 0x000278C4 File Offset: 0x00025CC4
	public void UpdateDynamicIDDict()
	{
		this.dynamicentityindex.Clear();
		this.islandindex.Clear();
		foreach (Entity entity in this.dynamicentities)
		{
			this.dynamicentityindex[entity.id] = entity;
			if (entity.type == EntType.island)
			{
				this.islandindex[entity.dat] = entity;
			}
		}
	}

	// Token: 0x060009D5 RID: 2517 RVA: 0x0002793C File Offset: 0x00025D3C
	public void UpdateSMap()
	{
		List<Entity> list = new List<Entity>(this.entities.Count);
		foreach (Entity entity in this.entities)
		{
			if (entity.type.Static())
			{
				list.Add(entity);
			}
		}
		if (list.Count == 0)
		{
			this.smapmin = new Coord(1, 1, 1);
			this.smapmax = new Coord(-1, -1, -1);
			this.staticmap = new Entity[0, 0, 0];
			return;
		}
		Coord pos = list[0].pos;
		int num = pos.x;
		int num2 = pos.y;
		int num3 = pos.z;
		int num4 = pos.x;
		int num5 = pos.y;
		int num6 = pos.z;
		for (int i = 1; i < list.Count; i++)
		{
			Coord pos2 = list[i].pos;
			num = Math.Min(num, pos2.x);
			num2 = Math.Min(num2, pos2.y);
			num3 = Math.Min(num3, pos2.z);
			num4 = Math.Max(num4, pos2.x);
			num5 = Math.Max(num5, pos2.y);
			num6 = Math.Max(num6, pos2.z);
		}
		this.staticmap = new Entity[num4 - num + 1, num5 - num2 + 1, num6 - num3 + 1];
		foreach (Entity entity2 in list)
		{
			this.staticmap[entity2.pos.x - num, entity2.pos.y - num2, entity2.pos.z - num3] = entity2;
		}
		this.smapmin = new Coord(num, num2, num3);
		this.smapmax = new Coord(num4, num5, num6);
	}

	// Token: 0x060009D6 RID: 2518 RVA: 0x00027B5C File Offset: 0x00025F5C
	public bool ActivelyForced(Entity ent)
	{
		return ent.movement != null;
	}

	// Token: 0x060009D7 RID: 2519 RVA: 0x00027B6C File Offset: 0x00025F6C
	public bool JustMoving(Entity ent)
	{
		Movement movement = ent.movement;
		return movement != null && movement.Starting();
	}

	// Token: 0x060009D8 RID: 2520 RVA: 0x00027B94 File Offset: 0x00025F94
	public bool Won()
	{
		bool flag = false;
		foreach (Entity entity in this.dynamicentities)
		{
			if (entity.type == EntType.sausage)
			{
				if (entity.pos.z <= 8)
				{
					if (entity.dat.Length <= 0 || entity.dat[0] != 'S')
					{
						if (entity.pos.z < -3)
						{
							return false;
						}
						flag = true;
						int num = entity.cookdata;
						for (int i = 0; i < 4; i++)
						{
							int num2 = num % 4;
							if (num2 == 0 || num2 == 3)
							{
								return false;
							}
							num /= 4;
						}
					}
				}
			}
		}
		return flag && this.Lost().Length <= 0;
	}

	// Token: 0x060009D9 RID: 2521 RVA: 0x00027C84 File Offset: 0x00026084
	public bool Falling(Entity e)
	{
		return !this.HasFooting(e) && e.dat.Length == 0 && (e.movement == null || (e.movement.direction == Direction.Down && e.movement.mtype == Movement.MType.Fall));
	}

	// Token: 0x060009DA RID: 2522 RVA: 0x00027CE4 File Offset: 0x000260E4
	public string Lost()
	{
		if (this.lostreason.Length > 0)
		{
			return this.lostreason;
		}
		if (this.player != null && this.player.pos.z < -2)
		{
			this.lostreason = "Drowned";
			return this.lostreason;
		}
		if (this.fork != null && this.fork.pos.z < -2)
		{
			this.lostreason = "Fork Lost";
			return this.lostreason;
		}
		foreach (Entity entity in this.dynamicentities)
		{
			if (entity.type == EntType.sausage)
			{
				if (entity.dat.Length > 0)
				{
					if (entity.dat[0] == 'B')
					{
						this.lostreason = "Burned";
						return this.lostreason;
					}
					if (entity.dat[0] == 'L')
					{
						this.lostreason = "Lost";
						return this.lostreason;
					}
				}
			}
		}
		return string.Empty;
	}

	// Token: 0x060009DB RID: 2523 RVA: 0x00027E08 File Offset: 0x00026208
	public static GameState NewState(MetaGameState _metagame)
	{
		GameState gameState = new GameState(_metagame);
		gameState.overworld = true;
		int num = 6;
		for (int i = 0; i < num; i++)
		{
			for (int j = 0; j < num; j++)
			{
				Entity entity = new Entity(gameState);
				entity.pos = new Coord(i, j, -2);
				entity.direction = (Direction)global::UnityEngine.Random.Range(0, 4);
				entity.type = EntType.ground;
				gameState.entities.Add(entity);
				if (entity.type.Dynamic())
				{
					entity._movement = new Movement();
					gameState.dynamicentities.Add(entity);
					gameState.dynamicentityindex.Add(entity.id, entity);
					if (entity.type == EntType.island)
					{
						gameState.islandindex[entity.dat] = entity;
					}
				}
			}
		}
		Entity entity2 = new Entity(gameState);
		entity2.pos = new Coord(num / 2, num / 2, -1);
		entity2.direction = (Direction)global::UnityEngine.Random.Range(0, 4);
		entity2.type = EntType.player;
		gameState.entities.Add(entity2);
		if (entity2.type.Dynamic())
		{
			entity2._movement = new Movement();
			gameState.dynamicentities.Add(entity2);
			gameState.dynamicentityindex.Add(entity2.id, entity2);
			if (entity2.type == EntType.island)
			{
				gameState.islandindex[entity2.dat] = entity2;
			}
		}
		gameState.player = entity2;
		gameState.dynamicentities.Sort((Entity e1, Entity e2) => e1.type.CompareTo(e2.type));
		gameState.UpdateSMap();
		gameState.idcounter = gameState.entities.Max<Entity>((Entity e) => e.id + 1);
		return gameState;
	}

	// Token: 0x060009DC RID: 2524 RVA: 0x00027FE5 File Offset: 0x000263E5
	public static GameState Blank(MetaGameState _metagame)
	{
		return new GameState(_metagame);
	}

	// Token: 0x060009DD RID: 2525 RVA: 0x00027FED File Offset: 0x000263ED
	private void BuildStaticCaches()
	{
		Entity.BuildOccupancyCache();
		GameState.BakEntitiesCacheGenerate();
	}

	// Token: 0x060009DE RID: 2526 RVA: 0x00027FFC File Offset: 0x000263FC
	public void AddEntity(Entity e, bool rebuildsmap)
	{
		if (e.type == EntType.fork && this.fork != null)
		{
			this.RemoveEntity(this.fork);
		}
		this.entitiesspawned = true;
		if (this.entities.Any<Entity>((Entity e2) => e2.id == e.id))
		{
			e.id = this.entities.Max<Entity>((Entity e2) => e2.id) + 1;
			this.SetIDCounterTo(e.id + 1);
		}
		this.entities.Add(e);
		if (e.type.Dynamic())
		{
			e._movement = new Movement();
			this.dynamicentities.Add(e);
			this.dynamicentityindex[e.id] = e;
			if (e.type == EntType.island)
			{
				this.islandindex[e.dat] = e;
			}
		}
		else if (rebuildsmap)
		{
			this.UpdateSMap();
		}
		if (e.type == EntType.player)
		{
			this.player = e;
		}
		else if (e.type == EntType.fork)
		{
			this.fork = e;
		}
		if (e.id >= this.idcounter)
		{
			this.idcounter = e.id + 1;
		}
		if (!e.Decoration())
		{
			this.BuildBox(e);
		}
	}

	// Token: 0x060009DF RID: 2527 RVA: 0x000281CE File Offset: 0x000265CE
	public Entity FromIDDynamic(int id)
	{
		return this.dynamicentityindex[id];
	}

	// Token: 0x060009E0 RID: 2528 RVA: 0x000281DC File Offset: 0x000265DC
	public Entity FromID(int id)
	{
		return this.entities.First<Entity>((Entity e) => e.id == id);
	}

	// Token: 0x060009E1 RID: 2529 RVA: 0x00028210 File Offset: 0x00026610
	public static void BakEntitiesCacheGenerate()
	{
		if (GameState.bakentindex < 0)
		{
			GameState.bakentsstock = new EntitySkeleton[10000];
			for (int i = 0; i < 10000; i++)
			{
				GameState.bakentsstock[i] = new EntitySkeleton();
			}
			GameState.bakentindex = 0;
		}
	}

	// Token: 0x060009E2 RID: 2530 RVA: 0x00028260 File Offset: 0x00026660
	public void BakEntities()
	{
		GameState._baklayer++;
		List<Entity> list = this.entities;
		if (GameState._baklayer >= GameState._bakskels.Count)
		{
			GameState._bakskels.Add(new EntitySkeleton[list.Count]);
			GameState._bakmovements.Add(new List<Movement>(250));
		}
		if (GameState._bakskels[GameState._baklayer].Length != list.Count)
		{
			GameState._bakskels[GameState._baklayer] = new EntitySkeleton[list.Count];
		}
		EntitySkeleton[] array = GameState._bakskels[GameState._baklayer];
		Array.Copy(GameState.bakentsstock, GameState.bakentindex, array, 0, list.Count);
		GameState.bakentindex += list.Count;
		for (int i = 0; i < list.Count; i++)
		{
			array[i].CopyFrom(list[i]);
		}
		List<Movement> list2 = GameState._bakmovements[GameState._baklayer];
		list2.Clear();
		list2.AddRange(this.movements);
		GameState.BakStruct bakStruct = new GameState.BakStruct(array, list2, null, null, this.pushtargetlevel, this.overworld, this.won, this.returning, this.haveevercookedall, this.tileset, this, this.lostreason);
		this.backup.Add(bakStruct);
	}

	// Token: 0x060009E3 RID: 2531 RVA: 0x000283B8 File Offset: 0x000267B8
	private void DiscardLastBackup()
	{
		GameState._baklayer--;
		GameState.bakentindex -= this.backup[this.backup.Count - 1].ents.Length;
		this.backup.RemoveAt(this.backup.Count - 1);
	}

	// Token: 0x060009E4 RID: 2532 RVA: 0x00028418 File Offset: 0x00026818
	public GameState.BakStruct TakeUndoSnapshot()
	{
		List<Entity> list = this.entities;
		int count = list.Count;
		EntitySkeleton[] array = new EntitySkeleton[count];
		for (int i = 0; i < count; i++)
		{
			array[i] = list[i].SaveToEntSkel();
		}
		List<Movement> list2 = new List<Movement>(this.movements.Count);
		for (int j = 0; j < this.movements.Count; j++)
		{
			list2.Add(this.movements[j].SaveToMovement());
		}
		string[] array2 = this.levelcompleted.ToArray();
		string[] array3 = this.worldsausagesissued.ToArray();
		GameState.BakStruct bakStruct = new GameState.BakStruct(array, list2, array2, array3, this.pushtargetlevel, this.overworld, this.won, this.returning, this.haveevercookedall, this.tileset, this, this.lostreason);
		return bakStruct;
	}

	// Token: 0x060009E5 RID: 2533 RVA: 0x000284FC File Offset: 0x000268FC
	public void RestoreEntities()
	{
		GameState.BakStruct bakStruct = this.backup[this.backup.Count - 1];
		this.DiscardLastBackup();
		this.RestoreSave(bakStruct, false);
	}

	// Token: 0x060009E6 RID: 2534 RVA: 0x00028530 File Offset: 0x00026930
	public void RestoreSave(GameState.BakStruct dat, bool clearstuff = true)
	{
		if (clearstuff)
		{
			this.moveattempts.Clear();
		}
		this.anyrolls = dat.anyrolls;
		this.anydrags = dat.anydrags;
		this.anyislandmoves = dat.anyislandmoves;
		this.anylandings = dat.anylandings;
		this.anyforklandings = dat.anyforklandings;
		this.playerwalk = dat.playerwalk;
		this.playerbackpedal = dat.playerbackpedal;
		this.playerturn = dat.playerturn;
		this.oldAshStepCount = dat.oldAshStepCount;
		this.ashStepCount = dat.ashStepCount;
		this.playerclimbup = dat.playerclimbup;
		this.playerclimbdownland = dat.playerclimbdownland;
		this.regenfork = dat.regenfork;
		this.addfork = dat.addfork;
		this.forkfork = dat.forkfork;
		this.doouch = dat.doouch;
		this.recalccoastsplashes = dat.recalccoastsplashes;
		this.startedrising = dat.startedrising;
		this.startedfalling = dat.startedfalling;
		this.donelowering = dat.donelowering;
		this.sausagelost = dat.sausagelost;
		this.forksfx = dat.forksfx;
		this.unforksfx = dat.unforksfx;
		this.attachforksfx = dat.attachforksfx;
		this.detatchforksfx = dat.detatchforksfx;
		this.drownsfx = dat.drownsfx;
		bool flag = this.fork == null;
		bool flag2 = false;
		this.exitPos = dat.exitPos;
		this.exitDir = dat.exitDir;
		this.exitUp = dat.exitUp;
		for (int i = 0; i < dat.ents.Length; i++)
		{
			EntitySkeleton entitySkeleton = dat.ents[i];
			if (entitySkeleton.type.Dynamic())
			{
				Entity entity = this.FromIDDynamic(entitySkeleton.id);
				if (entity == null)
				{
					entity = new Entity(this);
					entity.CopyFrom(entitySkeleton);
					entity.movement = null;
					if (entity.stuckto >= 0)
					{
						entity.stuckto = entitySkeleton.stuckto;
					}
					this.AddEntity(entity, false);
					this.entitiesspawned = false;
				}
				else
				{
					entity.CopyFrom(entitySkeleton);
					entity.movement = null;
				}
				if (entitySkeleton.type == EntType.sausage && entitySkeleton.pos == dat.exitTargetPos && dat.exitTargetPos != Coord.Invalid)
				{
					this.exitAttachment = entity;
				}
				if (entitySkeleton.type == EntType.fork)
				{
					flag2 = true;
					this.fork = entity;
					if (flag)
					{
						this.addfork = true;
						this.regenfork = true;
					}
				}
			}
			else
			{
				Entity entity2 = this.StaticEntAt(entitySkeleton.pos);
				if (entity2 == null)
				{
					entity2 = new Entity(this);
					entity2.CopyFrom(entitySkeleton);
					entity2.movement = null;
					if (entity2.stuckto >= 0)
					{
						entity2.stuckto = this.player.id;
						this.player.stuckto = entity2.id;
					}
					this.AddEntity(entity2, false);
					this.entitiesspawned = false;
				}
				else
				{
					entity2.CopyFrom(entitySkeleton);
					entity2.movement = null;
				}
			}
		}
		if (!flag && !flag2)
		{
			this.fork = null;
			this.player.cookdata = 0;
			this.player.Occupancy();
			this.regenfork = true;
		}
		if (dat.ents.Length != this.entities.Count)
		{
			for (int j = this.entities.Count - 1; j >= 0; j--)
			{
				Entity entity3 = this.entities[j];
				bool flag3 = false;
				for (int k = 0; k < dat.ents.Length; k++)
				{
					if (dat.ents[k].id == entity3.id)
					{
						flag3 = true;
					}
				}
				if (!flag3)
				{
					this.RemoveEntity(entity3);
				}
			}
		}
		this.movements.Clear();
		this.movements.AddRange(dat.movements);
		for (int l = 0; l < this.movements.Count; l++)
		{
			Movement movement = this.movements[l];
			movement.target = this.FromIDDynamic(movement.targetid);
			movement.target.movement = movement;
		}
		if (dat.levelcompleted != null)
		{
			this.levelcompleted.Clear();
			this.levelcompleted.AddRange(dat.levelcompleted);
		}
		if (dat.worldsausagesissued != null)
		{
			this.worldsausagesissued.Clear();
			this.worldsausagesissued.AddRange(dat.worldsausagesissued);
		}
		this.overworld = dat.overworld;
		this.won = dat.won;
		this.returning = dat.returning;
		this.haveevercookedall = dat.haveevercookedall;
		this.pushtargetlevel = dat.pushtargetlevel;
		this.tileset = dat.tileset;
		this.displayname = dat.displayname;
		this.musicseed = dat.musicseed;
		this.sausagescooked = dat.sausagescooked;
		this.lostreason = dat.lostreason;
	}

	// Token: 0x060009E7 RID: 2535 RVA: 0x00028A7C File Offset: 0x00026E7C
	public bool Moving()
	{
		return this.movements.Count > 0;
	}

	// Token: 0x060009E8 RID: 2536 RVA: 0x00028A8C File Offset: 0x00026E8C
	public Fraction MoveTickLength()
	{
		if (this.movements.Count != 0)
		{
			Fraction fraction = this.movements[0].remaining;
			for (int i = 1; i < this.movements.Count; i++)
			{
				Fraction remaining = this.movements[i].remaining;
				if (remaining < fraction)
				{
					fraction = remaining;
				}
			}
			return fraction;
		}
		if (this.Moving())
		{
			return 1;
		}
		return 0;
	}

	// Token: 0x060009E9 RID: 2537 RVA: 0x00028B14 File Offset: 0x00026F14
	public Fraction MaxSpeed(bool ignoreplayer)
	{
		if (this.movements.Count == 0)
		{
			return 1;
		}
		Fraction fraction = 1;
		for (int i = 0; i < this.movements.Count; i++)
		{
			Movement movement = this.movements[i];
			if (!ignoreplayer || movement.target != this.player)
			{
				if (movement.movetype != Movement.MoveType.None)
				{
					Fraction fraction2 = movement.speed;
					if (fraction2 > fraction)
					{
						fraction = fraction2;
					}
				}
			}
		}
		return fraction;
	}

	// Token: 0x060009EA RID: 2538 RVA: 0x00028BB0 File Offset: 0x00026FB0
	private void MovementsTick()
	{
		this.totrycook.Clear();
		Fraction fraction = this.MoveTickLength();
		bool flag = false;
		this.recalccoastsplashes = false;
		for (int i = this.movements.Count - 1; i >= 0; i--)
		{
			Movement movement = this.movements[i];
			movement.Tick(fraction);
			if (movement.remaining.num == 0)
			{
				if (movement.movetype == Movement.MoveType.None && (this.player.Turning() || (this.player.moving && this.player.movement.movetype == Movement.MoveType.Rotation)))
				{
					movement.SetSpeed(movement.speed);
				}
				else
				{
					movement.Resolve();
					movement.target.movement = null;
					this.movements.RemoveAt(i);
					if (movement.direction == Direction.Down)
					{
						if (movement.target.type == EntType.sausage)
						{
							this.CheckSausageLanded(movement.target);
						}
						else if (movement.target.type == EntType.fork)
						{
							this.CheckForkLanded(movement.target);
						}
						else if (movement.target.type == EntType.player)
						{
							this.CheckPlayerLanded(movement.target);
						}
					}
					if (movement.target.type == EntType.sausage)
					{
						this.totrycook.Add(movement.target);
					}
					else if (movement.target.type == EntType.island && movement.mtype != Movement.MType.Fixed)
					{
						flag = true;
						this.recalccoastsplashes = true;
					}
					if (movement.target.type == EntType.sausage && movement.target.pos.z < -2 && this.pushestotry == 0 && (movement.target.dat.Length == 0 || movement.target.dat[0] == 'M'))
					{
						movement.target.pos = new Coord(movement.target.pos.x, movement.target.pos.y, -100);
						if (movement.target.dat.Length == 0)
						{
							movement.target.dat = "L ; ; ";
						}
						else
						{
							movement.target.dat = 'L' + movement.target.dat.Substring(1);
						}
					}
				}
			}
		}
		if (this.pushestotry > 0)
		{
			this.TryLowerAll();
		}
		else if (this.pushestotry < 0)
		{
			this.TryRaiseAll();
		}
		if (flag)
		{
			foreach (Entity entity in this.dynamicentities)
			{
				this.DoCook(entity);
			}
		}
		else
		{
			foreach (Entity entity2 in this.totrycook)
			{
				this.DoCook(entity2);
			}
		}
	}

	// Token: 0x060009EB RID: 2539 RVA: 0x00028EBC File Offset: 0x000272BC
	private void CheckSausageLanded(Entity e)
	{
		Entity entity = this.EntAt(e.pos + Direction.Down, true, false);
		Entity entity2 = this.EntAt(e.pos + e.direction + Direction.Down, true, false);
		if ((entity != null && (entity.type == EntType.ground || entity.type == EntType.island) && !entity.moving) || (entity2 != null && (entity2.type == EntType.ground || entity2.type == EntType.island) && !entity2.moving))
		{
			Entity entity3 = e.LadenTarget();
			if (entity3 == null || entity3.type != EntType.player)
			{
				Coord coord = e.pos + Coord.Down;
				Coord coord2 = coord + e.direction;
				this.anylandings = Mathf.Max(this.anylandings, this.GetSurfaceType(coord));
				this.anylandings = Mathf.Max(this.anylandings, this.GetSurfaceType(coord2));
				this.landingPuffs.Add(coord);
				this.landingPuffs.Add(coord2);
			}
		}
	}

	// Token: 0x060009EC RID: 2540 RVA: 0x00028FD8 File Offset: 0x000273D8
	private void CheckForkLanded(Entity e)
	{
		Entity entity = this.EntAt(e.pos + Direction.Down, true, false);
		if (entity != null && (entity.type == EntType.ground || entity.type == EntType.island) && !entity.moving)
		{
			Entity entity2 = e.LadenTarget();
			if (entity2 == null || entity2.type != EntType.player)
			{
				Coord coord = e.pos + Coord.Down;
				this.anyforklandings = Mathf.Max(this.anyforklandings, this.GetSurfaceType(coord));
				this.landingPuffs.Add(coord);
			}
		}
	}

	// Token: 0x060009ED RID: 2541 RVA: 0x00029078 File Offset: 0x00027478
	private void CheckPlayerLanded(Entity e)
	{
		Entity entity = this.EntAt(e.pos + Direction.Down, true, false);
		if (entity != null && (entity.type == EntType.ground || entity.type == EntType.island) && !entity.moving)
		{
			Entity entity2 = e.LadenTarget();
			if (entity2 == null || entity2.type != EntType.player)
			{
				Coord coord = e.pos + Coord.Down;
				this.playerclimbdownland = Mathf.Max(this.playerclimbdownland, this.GetSurfaceType(coord));
			}
		}
	}

	// Token: 0x060009EE RID: 2542 RVA: 0x0002910C File Offset: 0x0002750C
	private int GetSurfaceType(Coord pos)
	{
		Entity entity = this.EntAt(pos, false, false);
		if (entity == null)
		{
			return 0;
		}
		int num;
		if (entity.type == EntType.island)
		{
			num = entity.IslandMaskVal(pos);
		}
		else
		{
			num = entity.FootprintType(this.tileset);
		}
		if (num == 1 || num == 2 || (num >= 13 && num <= 20))
		{
			GameState.lastftype = num;
		}
		else if (num >= 3 && num <= 6)
		{
			num = GameState.lastftype;
		}
		else
		{
			num = 0;
		}
		return num;
	}

	// Token: 0x060009EF RID: 2543 RVA: 0x00029198 File Offset: 0x00027598
	public bool ProcessInput(Direction dir)
	{
		if (this.player.movement != null)
		{
			return false;
		}
		if (this.player.pos.z < -2)
		{
			return false;
		}
		bool flag = true;
		if (this.Laden(this.player))
		{
			if (dir == this.player.direction || dir == this.player.direction.Inverse())
			{
				flag = this.TryMovePlayer(dir, false);
				if (flag)
				{
					Movement movement = this.player.movement;
					if (movement.mtype == Movement.MType.Backpedal || movement.mtype == Movement.MType.ForwardPedal)
					{
						this.playerbackpedal = this.GetSurfaceType(this.player.pos + dir + Coord.Down);
					}
					else if (movement.mtype == Movement.MType.WalkForward)
					{
						this.playerwalk = this.GetSurfaceType(this.player.pos + dir + Coord.Down);
					}
				}
			}
			else if (this.LadderUpInDir(dir))
			{
				flag = this.TryClimbUp(dir);
			}
			else if (this.LadderDownInDir(dir) && !this.SolidEntAt(this.player.pos + dir + Direction.Down, false, false))
			{
				flag = this.TryClimbDown(dir);
			}
			else
			{
				flag = this.TryMovePlayer(dir, false);
			}
		}
		else if (dir == this.player.direction || dir == this.player.direction.Inverse())
		{
			if (dir == this.player.direction && !this.player.Extended() && this.LadderUpInDir(dir))
			{
				flag = this.TryClimbUp(dir);
			}
			else if (dir == this.player.direction.Inverse() && !this.player.Extended() && this.LadderDownInDir(dir) && !this.SolidEntAt(this.player.pos + dir + Direction.Down, false, false))
			{
				this.TryClimbDown(dir);
			}
			else
			{
				flag = this.TryMovePlayer(dir, false);
				if (flag)
				{
					if (this.player.movement.mtype == Movement.MType.Backpedal)
					{
						this.playerbackpedal = this.GetSurfaceType(this.player.pos + dir + Coord.Down);
					}
					else
					{
						this.playerwalk = this.GetSurfaceType(this.player.pos + dir + Coord.Down);
					}
				}
			}
		}
		else if (this.player.Extended() && this.LadderUpInDir(dir))
		{
			flag = this.TryClimbUp(dir);
		}
		else if (this.player.Extended() && this.LadderDownInDir(dir) && !this.SolidEntAt(this.player.pos + dir + Direction.Down, false, false))
		{
			flag = this.TryClimbDown(dir);
		}
		else
		{
			flag = this.TryTurnPlayer(dir);
			this.playerturn = ((!flag) ? 0 : this.GetSurfaceType(this.player.pos + Coord.Down));
		}
		this.PassiveForceSweep(true);
		if (flag)
		{
			this.ProcessPetalStuff();
		}
		else
		{
			this.moveattempts[this.player.id] = dir;
		}
		return flag;
	}

	// Token: 0x060009F0 RID: 2544 RVA: 0x00029524 File Offset: 0x00027924
	public void CalcBBQAshSteps()
	{
		if (this.bbqsOn())
		{
			return;
		}
		Coord coord;
		int num2;
		int num = this.FootprintTypeAt(this.player.pos + Direction.Down, EnvironmentFader.global_target_tileset, out coord, out num2);
		if (num == 3)
		{
			this.oldAshStepCount = this.ashStepCount;
			this.ashStepCount = 4;
		}
	}

	// Token: 0x060009F1 RID: 2545 RVA: 0x00029580 File Offset: 0x00027980
	private void ProcessPetalStuff()
	{
		foreach (Movement movement in this.movements)
		{
			if (movement.Starting() && (movement.direction != Direction.None || movement.rotation))
			{
				if (movement.target.type == EntType.island)
				{
					if (movement.direction.Vertical())
					{
						this.SplashIsland(movement.target);
					}
				}
				else
				{
					foreach (Occupancy occupancy in movement.target.Occupancy())
					{
						if (occupancy.entering)
						{
							Coord pos = occupancy.pos;
							int num = this.DecorationAt(pos);
							if (num >= 0 && !this.petals.Contains(pos))
							{
								this.petals.Add(pos);
								this.petaltypes.Add(num);
							}
						}
					}
				}
			}
		}
	}

	// Token: 0x060009F2 RID: 2546 RVA: 0x0002968C File Offset: 0x00027A8C
	public bool LadderUpInDir(Direction dir)
	{
		Coord coord = this.player.pos + dir.ToCoord();
		return this.LadderAt(coord) == dir.Inverse();
	}

	// Token: 0x060009F3 RID: 2547 RVA: 0x000296C5 File Offset: 0x00027AC5
	public bool LadderDownInDir(Direction dir)
	{
		return this.LadderAt(this.player.pos + Direction.Down) == dir;
	}

	// Token: 0x060009F4 RID: 2548 RVA: 0x000296E8 File Offset: 0x00027AE8
	public Coord[] CombinedBorders(Entity e1, Entity e2, Direction dir)
	{
		Coord[] e1o = e1.SourceFootprint();
		Coord[] e2o = e2.SourceFootprint();
		Coord[] array = e1.Border(dir, true);
		Coord[] array2 = e2.Border(dir, true);
		return (from e in array.Concat<Coord>(array2)
			where !e1o.Contains(e) && !e2o.Contains(e)
			select e).ToArray<Coord>();
	}

	// Token: 0x060009F5 RID: 2549 RVA: 0x00029744 File Offset: 0x00027B44
	public Coord[] PlayerBorder(Direction dir, bool unfork)
	{
		if (unfork)
		{
			return this.player.Border(dir, true);
		}
		if (this.Laden(this.player))
		{
			return this.CombinedBorders(this.player, this.LadenTarget(this.player), dir);
		}
		return this.player.Border(dir, true);
	}

	// Token: 0x060009F6 RID: 2550 RVA: 0x000297A0 File Offset: 0x00027BA0
	private void AddRotation(Entity e, Direction from, Direction to, Movement.MType mtype, int speed)
	{
		Movement movement = Movement.Rotation(e, from, to, mtype, speed);
		this.movements.Add(movement);
		e.movement = movement;
	}

	// Token: 0x060009F7 RID: 2551 RVA: 0x000297D0 File Offset: 0x00027BD0
	private void AddPivot(Entity e, Direction movedir, Direction from, Direction to, Movement.MType mtype, int speed)
	{
		Movement movement = Movement.Pivot(e, movedir, from, to, mtype, speed);
		this.movements.Add(movement);
		e.movement = movement;
	}

	// Token: 0x060009F8 RID: 2552 RVA: 0x000297FF File Offset: 0x00027BFF
	private void AddTranslationOK(Entity e, Direction dir, int torsion, int speed)
	{
		if (dir.Vertical())
		{
			this.AddTranslation(e, dir, torsion, speed, Movement.MType.Fall, true, true);
		}
		else
		{
			this.AddTranslation(e, dir, torsion, speed, Movement.MType.Idle, true, true);
		}
	}

	// Token: 0x060009F9 RID: 2553 RVA: 0x00029830 File Offset: 0x00027C30
	private void AddTranslation(Entity e, Direction dir, int torsion, int speed, Movement.MType mtype, bool left, bool recursive = true)
	{
		if (e.movement != null)
		{
			return;
		}
		if (torsion != 0 && torsion != -666 && dir.Horizontal())
		{
			this.anyrolls = Mathf.Max(this.anyrolls, this.GetSurfaceType(e.pos + Coord.Down));
			this.anyrolls = Mathf.Max(this.anyrolls, this.GetSurfaceType(e.pos + e.direction + Coord.Down));
		}
		else if (e.type == EntType.sausage && torsion != -666 && dir.Horizontal())
		{
			this.anydrags = Mathf.Max(this.anydrags, this.GetSurfaceType(e.pos + Coord.Down));
			this.anydrags = Mathf.Max(this.anydrags, this.GetSurfaceType(e.pos + e.direction + Coord.Down));
		}
		if (e.type == EntType.island && dir.Horizontal())
		{
			Coord[] array = e.Border(Direction.Down, true);
			List<Entity> list = this.EntsAt(array, false);
			foreach (Entity entity in list)
			{
				if (entity.movement == null || entity.movement.speed != speed)
				{
					this.anyislandmoves = 1;
					break;
				}
			}
		}
		Movement movement = Movement.Translation(e, dir, torsion, speed, mtype, left);
		this.movements.Add(movement);
		e.movement = movement;
		if (recursive && this.Laden(e))
		{
			Entity entity2 = this.LadenTarget(e);
			if (entity2.type == EntType.player && dir == Direction.Down)
			{
				mtype = Movement.MType.Fall;
			}
			this.AddTranslation(entity2, dir, torsion, speed, mtype, left, false);
		}
	}

	// Token: 0x060009FA RID: 2554 RVA: 0x00029A1C File Offset: 0x00027E1C
	public bool TryMovePlayer(Direction dir, bool unfork = false)
	{
		if (this.player.pos.z < -2)
		{
			return false;
		}
		this.player.dat = string.Empty;
		this.lastdir = dir;
		this.BakEntities();
		bool flag = false;
		Entity entity = this.Floor(this.player, false);
		if (entity == null)
		{
			this.DiscardLastBackup();
			return false;
		}
		Entity entity2 = this.Floor(this.player.pos + dir, false);
		if (entity2 != null && entity2.type == EntType.island)
		{
			if (this.LadderAt(this.player.pos + Direction.Down) != dir)
			{
				entity2.movement = Movement.Fixed(entity2, 1);
				this.movements.Add(entity2.movement);
			}
		}
		bool flag2 = false;
		if (entity.type == EntType.sausage && dir.NormalTo(entity.direction))
		{
			flag2 = this.ApplyForce(this.player.pos + Coord.Down, dir.Inverse(), 1, 1, false, null, false);
			if (!flag2)
			{
				this.moveattempts[entity.id] = dir.Inverse();
			}
			this.PassiveForceSweep(true);
			Movement movement = entity.movement;
			if (movement != null && movement.torsion != 1)
			{
				movement.torsion = 1;
			}
			bool flag3 = movement != null && movement.torsion != 0;
			if (flag3)
			{
				flag3 = false;
				List<Entity> list = this.EntsAt(entity.SourceFootprintLower(), true);
				for (int i = 0; i < list.Count; i++)
				{
					Entity entity3 = list[i];
					if (!entity3.moving)
					{
						flag3 = true;
					}
				}
			}
			if (flag3)
			{
				flag = true;
				dir = dir.Inverse();
				if (flag2 && movement.direction == dir.Inverse())
				{
					this.DiscardLastBackup();
					this.BakEntities();
				}
			}
			else
			{
				this.RestoreEntities();
				this.BakEntities();
			}
		}
		Movement movement2 = Movement.Translation(this.player, dir, 0, 1, Movement.MType.ForwardPedal, false);
		this.player.movement = movement2;
		this.movements.Add(movement2);
		if (this.player.Laden() && !unfork)
		{
			Entity entity4 = this.player.LadenTarget();
			entity4.movement = Movement.Translation(entity4, dir, 0, 1, Movement.MType.ForwardPedal, false);
			this.movements.Add(entity4.movement);
		}
		this.ApplyForce(this.player, dir, 1, 1, !unfork, false);
		this.movements.Remove(movement2);
		this.player.movement = null;
		if (this.player.Laden() && !unfork)
		{
			Entity entity5 = this.player.LadenTarget();
			this.movements.Remove(entity5.movement);
			entity5.movement = null;
		}
		Movement.MType mtype = ((!flag) ? Movement.MType.WalkForward : Movement.MType.ForwardPedal);
		bool flag4 = dir.LeftOf(this.player.direction);
		if (dir == this.player.direction.Inverse())
		{
			mtype = Movement.MType.Backpedal;
		}
		if (dir.NormalTo(this.player.direction))
		{
			if (dir.LeftOf(this.player.direction))
			{
				mtype = Movement.MType.StrafeL;
			}
			else
			{
				mtype = Movement.MType.StrafeR;
			}
		}
		this.AddTranslation(this.player, dir, 0, 1, mtype, flag4, !unfork);
		if (dir == this.player.direction)
		{
			this.TryFork();
		}
		bool flag5 = ((!flag2) ? this.StableGround(this.player.TargetPos()) : this.Floating(this.player));
		if (flag5)
		{
			this.moveattempts[this.player.id] = dir;
			if (this.player.stuckto >= 0)
			{
				this.moveattempts[this.player.stuckto] = dir;
			}
		}
		bool flag6 = this.player.Collides(true, false);
		if (flag6 || flag5)
		{
			this.RestoreEntities();
			if (!unfork && this.Laden(this.player) && dir == this.player.direction.Inverse())
			{
				return this.TryMovePlayer((!flag) ? dir : dir.Inverse(), true);
			}
			if (flag && flag5)
			{
				Entity entity6 = this.EntAt(this.player.pos + Coord.Down, false, false);
				if (entity6 != null && !entity6.direction.ParallelTo(this.player.direction))
				{
					Movement movement3 = Movement.Surprise(this.player, dir, Movement.MType.Surprise_Chasm);
					this.movements.Add(movement3);
					this.player.movement = movement3;
					return true;
				}
			}
			if (flag && entity.moving)
			{
				Movement movement4 = Movement.Translation(this.player, Direction.None, 0, 1, Movement.MType.ForwardPedal, false);
				this.movements.Add(movement4);
				this.player.movement = movement4;
				return true;
			}
			if (flag6)
			{
				Coord[] array = this.PlayerBorder(dir, false);
				List<Entity> list2 = this.EntsAt(array, true);
				if (this.player.Laden() && !unfork)
				{
					this.BakEntities();
					bool flag7 = this.nopassivesweeps;
					this.nopassivesweeps = true;
					foreach (Entity entity7 in list2)
					{
						if (entity7 != this.player && entity7.id != this.player.stuckto)
						{
							this.TryPushEnt(entity7, dir, 0, 1, false, true, false, false);
							if (!entity7.moving)
							{
								this.moveattempts[entity7.id] = dir;
							}
						}
					}
					this.nopassivesweeps = flag7;
					this.RestoreEntities();
				}
				else
				{
					foreach (Entity entity8 in list2)
					{
						if (entity8 != this.player && entity8.id != this.player.stuckto)
						{
							this.moveattempts[entity8.id] = dir;
						}
					}
				}
			}
			return false;
		}
		else
		{
			if (!entity.moving && entity2 == null)
			{
				this.RestoreEntities();
				return false;
			}
			if (this.LadderAt(this.player.pos + Coord.Down) == dir)
			{
				List<Entity> list3 = this.EntsAt_NoAlloc(this.player.pos + Coord.Down + dir, false, false);
				bool flag8 = false;
				for (int j = 0; j < list3.Count; j++)
				{
					Entity entity9 = list3[j];
					if (entity9.type == EntType.island && entity9.movement != null && entity9.movement.direction == dir)
					{
						flag8 = true;
						break;
					}
				}
				if (flag8)
				{
					this.RestoreEntities();
					this.TryClimbDown(dir);
					return true;
				}
			}
			this.DiscardLastBackup();
			if (unfork)
			{
				this.unforksfx = true;
				Entity entity10 = this.FromIDDynamic(this.player.stuckto);
				this.player.stuckto = -1;
				entity10.stuckto = -1;
			}
			return true;
		}
	}

	// Token: 0x060009FB RID: 2555 RVA: 0x0002A17C File Offset: 0x0002857C
	private void TryFork()
	{
		if (this.player.stuckto >= 0 || !this.player.Extended())
		{
			return;
		}
		if (this.player.movement == null || !this.player.movement.translation || this.player.movement.direction != this.player.direction)
		{
			return;
		}
		Coord coord = this.player.TargetPos() + this.player.direction;
		List<Entity> list = this.EntsAt_NoAlloc(coord, false, false);
		bool flag = false;
		Entity entity = null;
		foreach (Entity entity2 in list)
		{
			if (entity2.type == EntType.sausage && !this.ActivelyForced(entity2))
			{
				flag = true;
				entity = entity2;
				break;
			}
		}
		if (flag && !entity.At(this.player.TargetPos(), true, true, false))
		{
			this.player.stuckto = entity.id;
			entity.stuckto = this.player.id;
			this.forksfx = true;
		}
	}

	// Token: 0x060009FC RID: 2556 RVA: 0x0002A2AC File Offset: 0x000286AC
	private void ApplyPivotForces1(Entity e, Direction movedir, Direction fromdir, Direction todir)
	{
		if (fromdir.Ortho())
		{
			if (fromdir != movedir)
			{
				if (fromdir != movedir.Inverse())
				{
					if (DirectionUtil.RotBetween(fromdir, movedir) == todir)
					{
						bool flag;
						this.ApplyForce(e.pos + todir, fromdir, 1, 2, out flag, true, e);
					}
				}
			}
		}
		else if (todir != movedir)
		{
			if (todir != movedir.Inverse())
			{
				if (DirectionUtil.ContinueRot(todir, fromdir) != movedir)
				{
					bool flag;
					this.ApplyForce(e.pos + todir, todir, 1, 2, out flag, true, e);
				}
			}
		}
	}

	// Token: 0x060009FD RID: 2557 RVA: 0x0002A364 File Offset: 0x00028764
	private void ApplyPivotForces2(Entity e, Direction movedir, Direction fromdir, Direction todir)
	{
		if (fromdir.Ortho())
		{
			if (fromdir == movedir)
			{
				bool flag;
				this.ApplyForce(e.pos + movedir + fromdir, movedir, 1, 1, out flag, true, e);
				this.ApplyForce(e.pos + movedir + todir, movedir, 1, 1, out flag, true, e);
			}
			else if (fromdir == movedir.Inverse())
			{
				bool flag;
				this.ApplyForce(e.pos + movedir, movedir, 1, 1, out flag, true, e);
				this.ApplyForce(e.pos + movedir + todir, movedir, 1, 1, out flag, true, e);
			}
			else if (DirectionUtil.RotBetween(fromdir, movedir) == todir)
			{
				bool flag;
				this.ApplyForce(e.pos + movedir, movedir, 1, 1, out flag, true, e);
				this.ApplyForce(e.pos + movedir + todir, movedir, 1, 1, out flag, true, e);
			}
			else
			{
				bool flag;
				this.ApplyForce(e.pos + movedir, movedir, 1, 1, out flag, true, e);
			}
		}
		else if (todir == movedir)
		{
			bool flag;
			this.ApplyForce(e.pos + movedir + fromdir, movedir, 1, 1, out flag, true, e);
			this.ApplyForce(e.pos + movedir + todir, movedir, 1, 1, out flag, true, e);
			Direction direction = DirectionUtil.ContinueRot(todir, fromdir).Inverse();
			this.ApplyForce(e.pos + movedir, direction, 1, 1, out flag, true, e);
		}
		else if (todir == movedir.Inverse())
		{
			Direction direction2 = DirectionUtil.ContinueRot(todir, fromdir).Inverse();
			bool flag;
			this.ApplyForce(e.pos + movedir, movedir, 1, 1, out flag, true, e);
			this.ApplyForce(e.pos - movedir, direction2, 1, 2, out flag, true, e);
		}
		else if (DirectionUtil.ContinueRot(todir, fromdir) == movedir)
		{
			bool flag;
			this.ApplyForce(e.pos + movedir, movedir, 1, 1, out flag, true, e);
		}
		else
		{
			bool flag;
			this.ApplyForce(e.pos + movedir, movedir, 1, 1, out flag, true, e);
			this.ApplyForce(e.pos + movedir + todir, movedir, 1, 1, out flag, true, e);
		}
	}

	// Token: 0x060009FE RID: 2558 RVA: 0x0002A5B4 File Offset: 0x000289B4
	private bool TryPivotTurn(Entity e, bool clockwise, Direction pushdir = Direction.None)
	{
		if (e.movement != null)
		{
			return false;
		}
		Entity entity = this.Floor(this.player, false);
		if (entity == null || entity.type != EntType.island)
		{
			return false;
		}
		this.BakEntities();
		if (e == this.player && entity.movement != null && entity.movement.movetype == Movement.MoveType.None)
		{
			this.movements.Remove(entity.movement);
			entity.movement = null;
		}
		Direction direction = e.direction.Rot90(clockwise);
		Direction direction2 = DirectionUtil.RotBetween(e.direction, direction);
		if (pushdir == Direction.None)
		{
			pushdir = direction.Inverse();
		}
		bool flag = true;
		Movement movement = Movement.Pivot(e, pushdir, e.direction, direction2, Movement.MType.TurnIn, 1);
		e.movement = movement;
		this.movements.Add(e.movement);
		this.ApplyPivotForces1(e, pushdir, e.direction, direction2);
		if (e == this.player)
		{
			flag = this.ApplyForce(e.pos + Coord.Down, pushdir, 0, 1, false, null, true);
			if (!flag)
			{
				this.moveattempts[entity.id] = pushdir;
			}
		}
		this.ApplyPivotForces2(e, pushdir, e.direction, direction2);
		this.movements.Remove(e.movement);
		e.movement = null;
		Entity hat = this.GetHat(e);
		if (hat != null)
		{
			this.TryPivotTurn(hat, clockwise, pushdir);
		}
		this.PassiveForceSweep(true);
		this.AddPivot(e, pushdir, e.direction, direction2, Movement.MType.TurnIn, 1);
		e.turndir = direction;
		if (!flag || e.Collides(true, false))
		{
			this.RestoreEntities();
			return e.type == EntType.sausage && this.TryPushEnt(e, pushdir, 0, 1, false, true, false, false);
		}
		this.DiscardLastBackup();
		return true;
	}

	// Token: 0x060009FF RID: 2559 RVA: 0x0002A77C File Offset: 0x00028B7C
	private bool AutomaticPivotTurn(Entity e, Direction pushdir = Direction.None)
	{
		if (e.movement != null)
		{
			return false;
		}
		Entity entity = this.Floor(this.player, false);
		if (entity == null || entity.type != EntType.island)
		{
			return false;
		}
		this.BakEntities();
		if (e == this.player && entity.movement != null && entity.movement.movetype == Movement.MoveType.None)
		{
			this.movements.Remove(entity.movement);
			entity.movement = null;
		}
		Direction direction = e.direction;
		Direction turndir = e.turndir;
		Direction direction2 = DirectionUtil.ContinueRot(turndir, direction);
		if (pushdir == Direction.None)
		{
			pushdir = direction2;
		}
		Movement movement = Movement.Pivot(e, pushdir, e.direction, turndir, Movement.MType.TurnOut, 1);
		e.movement = movement;
		this.movements.Add(movement);
		this.ApplyPivotForces1(e, pushdir, direction, turndir);
		bool flag = true;
		if (e == this.player)
		{
			flag = this.ApplyForce(e.pos + Coord.Down, pushdir, 0, 1, false, null, true);
			if (!flag)
			{
				this.moveattempts[entity.id] = pushdir;
			}
		}
		this.ApplyPivotForces2(e, pushdir, direction, turndir);
		Entity hat = this.GetHat(e);
		if (hat != null)
		{
			if (hat.Turning())
			{
				if (hat.movement != null)
				{
					this.movements.Remove(hat.movement);
					hat.movement = null;
				}
				this.AutomaticPivotTurn(hat, pushdir);
			}
			else
			{
				this.TryPushEnt(hat, pushdir, 0, 1, false, false, false, false);
			}
		}
		this.movements.Remove(e.movement);
		e.movement = null;
		this.PassiveForceSweep(true);
		this.AddPivot(e, pushdir, e.direction, turndir, Movement.MType.TurnOut, 1);
		if (!flag || e.Collides(true, false))
		{
			this.RestoreEntities();
			if (e.type != EntType.sausage)
			{
				return false;
			}
			this.AutomaticPivotTurnBack(e, pushdir, direction2);
		}
		else
		{
			this.DiscardLastBackup();
		}
		e.turndir = Direction.None;
		return true;
	}

	// Token: 0x06000A00 RID: 2560 RVA: 0x0002A97C File Offset: 0x00028D7C
	private bool AutomaticPivotTurnBack(Entity e, Direction pushdir, Direction targetdir)
	{
		if (e.movement != null)
		{
			return false;
		}
		Entity entity = this.Floor(this.player, false);
		if (entity == null || entity.type != EntType.island)
		{
			return false;
		}
		this.BakEntities();
		Direction direction = e.direction;
		Direction direction2 = DirectionUtil.ContinueRot(targetdir, direction);
		if (pushdir == Direction.None)
		{
			pushdir = direction2;
		}
		Movement movement = Movement.Pivot(e, pushdir, e.direction, targetdir, Movement.MType.TurnOut, 1);
		e.movement = movement;
		this.movements.Add(e.movement);
		this.ApplyPivotForces1(e, pushdir, direction, targetdir);
		bool flag = true;
		if (e == this.player)
		{
			flag = this.ApplyForce(e.pos + Coord.Down, pushdir, 0, 1, false, null, true);
		}
		this.ApplyPivotForces2(e, pushdir, direction, targetdir);
		Entity hat = this.GetHat(e);
		if (hat != null)
		{
			if (hat.Turning())
			{
				if (hat.movement != null)
				{
					this.movements.Remove(hat.movement);
					hat.movement = null;
				}
				this.AutomaticPivotTurn(hat, pushdir);
			}
			else
			{
				this.TryPushEnt(hat, pushdir, 0, 1, false, false, false, false);
			}
		}
		this.movements.Remove(e.movement);
		e.movement = null;
		this.PassiveForceSweep(true);
		this.AddPivot(e, pushdir, e.direction, targetdir, Movement.MType.TurnOut, 1);
		if (!flag || e.Collides(true, false))
		{
			this.RestoreEntities();
			Debug.LogError("automaticpivotturnback failed :( ");
			this.RotateBack(e);
		}
		else
		{
			this.DiscardLastBackup();
		}
		e.turndir = Direction.None;
		if (e == this.player)
		{
			this.moveattempts[this.idcounter] = pushdir;
		}
		return true;
	}

	// Token: 0x06000A01 RID: 2561 RVA: 0x0002AB34 File Offset: 0x00028F34
	public bool TryTurn(Entity e, bool clockwise, int turnspeed = 2)
	{
		if (e.movement != null)
		{
			return false;
		}
		Direction direction = e.direction.Rot90(clockwise);
		Direction direction2 = DirectionUtil.RotBetween(e.direction, direction);
		Coord coord = e.pos + direction2;
		this.BakEntities();
		Entity hat = this.GetHat(e);
		if (hat != null)
		{
			turnspeed = 1;
		}
		Movement movement = Movement.Rotation(e, e.direction, direction2, Movement.MType.TurnIn, 1);
		e.movement = movement;
		this.movements.Add(e.movement);
		Entity entity = null;
		if (e == this.player)
		{
			entity = this.Floor(this.player, false);
			if (entity != null && !entity.type.Static())
			{
				this.movements.Remove(entity.movement);
				entity.movement = Movement.Fixed(entity, turnspeed);
				this.movements.Add(entity.movement);
			}
		}
		if (e.Extended())
		{
			if (this.ApplyForce(coord, direction, 1, 1, false, null, false))
			{
				turnspeed = 1;
			}
			else if (e == this.player)
			{
				List<Entity> list = this.EntsAt_NoAlloc(coord, true, false);
				foreach (Entity entity2 in list)
				{
					if (entity2 != this.player)
					{
						this.moveattempts[entity2.id] = direction;
					}
				}
			}
		}
		this.movements.Remove(e.movement);
		e.movement = null;
		if (entity != null && entity.moving)
		{
			entity.movement.SetSpeed(turnspeed);
		}
		this.AddRotation(e, e.direction, direction2, Movement.MType.TurnIn, turnspeed);
		e.turndir = direction;
		if (!e.Collides(true, false))
		{
			this.DiscardLastBackup();
			if (hat != null)
			{
				this.PassiveForceSweep(true);
				this.TryTurn(hat, clockwise, turnspeed);
			}
			return true;
		}
		this.RestoreEntities();
		if (e == this.player)
		{
			bool flag = this.TryPivotTurn(e, clockwise, Direction.None);
			if (!flag)
			{
				this.moveattempts[e.id] = direction;
			}
			return flag;
		}
		return false;
	}

	// Token: 0x06000A02 RID: 2562 RVA: 0x0002AD58 File Offset: 0x00029158
	public bool TryTurnPlayer(Direction dir)
	{
		this.player.dat = string.Empty;
		return this.TryTurn(this.player, this.player.direction.LeftOf(dir), 2);
	}

	// Token: 0x06000A03 RID: 2563 RVA: 0x0002AD98 File Offset: 0x00029198
	public Direction LadderAt(Coord c)
	{
		Entity entity = this.EntAt(c, false, false);
		if (entity == null)
		{
			return Direction.None;
		}
		if (entity.type == EntType.ladder)
		{
			return entity.direction;
		}
		if (entity.type == EntType.island)
		{
			int num = entity.IslandMaskVal(c);
			if (num >= 3 && num <= 6)
			{
				return (Direction)(num - 3);
			}
		}
		return Direction.None;
	}

	// Token: 0x06000A04 RID: 2564 RVA: 0x0002ADF4 File Offset: 0x000291F4
	public Direction LadderAt(Coord c, out Entity ladder)
	{
		Entity entity = this.EntAt(c, false, false);
		if (entity == null)
		{
			ladder = null;
			return Direction.None;
		}
		if (entity.type == EntType.ladder)
		{
			ladder = entity;
			return entity.direction;
		}
		if (entity.type == EntType.island)
		{
			int num = entity.IslandMaskVal(c);
			if (num >= 3 && num <= 6)
			{
				ladder = entity;
				return (Direction)(num - 3);
			}
		}
		ladder = null;
		return Direction.None;
	}

	// Token: 0x06000A05 RID: 2565 RVA: 0x0002AE5C File Offset: 0x0002925C
	public int FootprintTypeAt(Coord c, int islandtileset, out Coord offset, out int islandid)
	{
		HashSet<Entity> hashSet = this.BoxAt(c);
		if (hashSet != null)
		{
			foreach (Entity entity in hashSet)
			{
				if (entity.At(c, false, false, true))
				{
					if (entity.type == EntType.ground || entity.type == EntType.ladder)
					{
						offset = Coord.Zero;
						islandid = -1;
						int num = entity.FootprintType(islandtileset);
						if (num == 13)
						{
							return 1;
						}
						if (num == 14)
						{
							return 2;
						}
						return 0;
					}
					else
					{
						if (entity.type == EntType.bbq)
						{
							offset = Coord.Zero;
							islandid = -1;
							return 3;
						}
						if (entity.type == EntType.island)
						{
							offset = entity.pos;
							islandid = entity.id;
							if (!entity.moving)
							{
								int num2 = entity.IslandMaskVal(c);
								if (num2 == 13)
								{
									return 1;
								}
								if (num2 == 14)
								{
									return 2;
								}
								if (num2 == 20 || num2 == 2)
								{
									return 3;
								}
								return 0;
							}
						}
					}
				}
			}
		}
		islandid = -1;
		offset = Coord.Zero;
		return 0;
	}

	// Token: 0x06000A06 RID: 2566 RVA: 0x0002AF7C File Offset: 0x0002937C
	public int DecorationAt(Coord c)
	{
		HashSet<Entity> hashSet = this.BoxAt(c);
		if (hashSet == null)
		{
			return 0;
		}
		foreach (Entity entity in hashSet)
		{
			if (entity.At(c, false, false, true))
			{
				if (entity.type == EntType.ground && entity.Decoration())
				{
					return entity.DecorationType();
				}
				if (entity.type == EntType.island && !entity.moving)
				{
					int num = entity.IslandMaskVal(c);
					if (num <= -10)
					{
						return -10 - num;
					}
				}
			}
		}
		return -1;
	}

	// Token: 0x06000A07 RID: 2567 RVA: 0x0002B018 File Offset: 0x00029418
	public bool BBQAt(Coord c)
	{
		if (!this.bbqsOn())
		{
			return false;
		}
		Entity entity = this.EntAt(c, false, false);
		if (entity == null)
		{
			return false;
		}
		if (entity.type == EntType.bbq)
		{
			return true;
		}
		if (entity.type == EntType.island)
		{
			int num = entity.IslandMaskVal(c);
			return num == 2 || num == 20;
		}
		return false;
	}

	// Token: 0x06000A08 RID: 2568 RVA: 0x0002B078 File Offset: 0x00029478
	public Direction BBQAtDir(Coord c, out string bbqdatstring)
	{
		Entity entity = this.EntAt(c, false, false);
		if (entity == null)
		{
			bbqdatstring = string.Empty;
			return Direction.None;
		}
		if (entity.type == EntType.bbq)
		{
			bbqdatstring = entity.id.ToStringFast();
			return entity.direction;
		}
		if (entity.type == EntType.island)
		{
			Coord coord;
			int num = entity.IslandMaskVal(c, out coord);
			if (num == 2)
			{
				bbqdatstring = string.Concat(new string[]
				{
					entity.dat,
					".",
					coord.x.ToStringFast(),
					".",
					coord.y.ToStringFast(),
					".",
					coord.z.ToStringFast()
				});
				return Direction.East;
			}
			if (num == 20)
			{
				bbqdatstring = string.Concat(new string[]
				{
					entity.dat,
					".",
					coord.x.ToStringFast(),
					".",
					coord.y.ToStringFast(),
					".",
					coord.z.ToStringFast()
				});
				return Direction.North;
			}
		}
		bbqdatstring = " ";
		return Direction.None;
	}

	// Token: 0x06000A09 RID: 2569 RVA: 0x0002B1A8 File Offset: 0x000295A8
	public bool BBQAt(Coord c, out string bbqdatstring)
	{
		Entity entity = this.EntAt(c, false, false);
		if (entity == null)
		{
			bbqdatstring = string.Empty;
			return false;
		}
		if (entity.type == EntType.bbq)
		{
			bbqdatstring = entity.dat;
			return true;
		}
		if (entity.type == EntType.island)
		{
			Coord coord;
			int num = entity.IslandMaskVal(c, out coord);
			bbqdatstring = string.Concat(new object[] { entity.dat, ".", coord.x, ".", coord.y, ".", coord.z });
			return num == 2 || num == 20;
		}
		bbqdatstring = " ";
		return false;
	}

	// Token: 0x06000A0A RID: 2570 RVA: 0x0002B270 File Offset: 0x00029670
	public Direction PedastalAt(Coord c)
	{
		Entity entity = this.EntAt(c, false, false);
		if (entity == null)
		{
			return Direction.None;
		}
		if (entity.type == EntType.island)
		{
			int num = entity.IslandMaskVal(c);
			if (num >= 9 && num <= 12)
			{
				return (Direction)(num - 9);
			}
		}
		return Direction.None;
	}

	// Token: 0x06000A0B RID: 2571 RVA: 0x0002B2BC File Offset: 0x000296BC
	public Entity PedastalAt(Coord[] c, out Direction dir, out Coord pos)
	{
		List<Entity> list = this.EntsAt_NoAlloc(c, false);
		foreach (Entity entity in list)
		{
			if (entity != null)
			{
				if (entity.type == EntType.island)
				{
					foreach (Coord coord in c)
					{
						int num = entity.IslandMaskVal(coord);
						if (num >= 9 && num <= 12)
						{
							dir = (Direction)(num - 9);
							pos = coord;
							return entity;
						}
					}
				}
			}
		}
		pos = Coord.Invalid;
		dir = Direction.None;
		return null;
	}

	// Token: 0x06000A0C RID: 2572 RVA: 0x0002B370 File Offset: 0x00029770
	public bool TryClimbUp(Direction dir)
	{
		this.lastdir = dir;
		this.BakEntities();
		Coord coord = this.player.pos + dir;
		Entity entity;
		this.LadderAt(coord, out entity);
		Entity entity2;
		Movement.MType mtype = ((this.LadderAt(coord + Direction.Up, out entity2) != dir.Inverse()) ? Movement.MType.ClimbUp_End1 : Movement.MType.ClimbUp_Init);
		this.AddTranslation(this.player, Direction.Up, 0, 1, mtype, dir.LeftOf(this.player.direction), true);
		this.player.dat = ((int)dir).ToStringFast();
		this.ApplyForce(this.player, Direction.Up, 0, 1, true, false);
		if (this.player.Collides(true, false) || (entity != null && entity.movement != null))
		{
			this.RestoreEntities();
			return false;
		}
		this.DiscardLastBackup();
		this.playerclimbup = true;
		return true;
	}

	// Token: 0x06000A0D RID: 2573 RVA: 0x0002B44C File Offset: 0x0002984C
	public bool TryClimbDown(Direction dir)
	{
		this.lastdir = dir;
		this.BakEntities();
		this.AddTranslation(this.player, dir, 0, 1, Movement.MType.ClimbDown_Init1, dir.LeftOf(this.player.direction), true);
		this.ApplyForce(this.player, dir, 1, 1, true, false);
		if (dir == this.player.direction)
		{
			this.TryFork();
		}
		this.player.dat = ((int)((Direction)(-1) - dir)).ToString();
		if (this.player.Collides(true, false))
		{
			this.RestoreEntities();
			return false;
		}
		this.DiscardLastBackup();
		return true;
	}

	// Token: 0x06000A0E RID: 2574 RVA: 0x0002B4F0 File Offset: 0x000298F0
	public bool TryPushEnt(Entity e, Direction dir, int torsion, int speed, bool weakforce = false, bool forcezerotorsion = false, bool canchangeplayerfooting = false, bool passive = false)
	{
		if (e.type.Static() || e.moving || e.type == EntType.barrier)
		{
			return false;
		}
		if (dir.Vertical())
		{
			forcezerotorsion = true;
		}
		if (e.type == EntType.island)
		{
			if (this.overworld && this.pushestotry == 0)
			{
				return false;
			}
			if (this.pushtargetlevel == e.dat)
			{
				return false;
			}
		}
		if (!canchangeplayerfooting && e.type == EntType.island && this.Footing(this.player) == e)
		{
			if (!dir.Vertical())
			{
				return false;
			}
		}
		this.BakEntities();
		List<Entity> list = this.Under(e, true);
		if (!e.type.CanRoll() || list.Count == 0)
		{
			torsion = 0;
		}
		else
		{
			Movement movement = list[0].movement;
			if (movement == null)
			{
				torsion = 0;
			}
			else
			{
				torsion = movement.torsion;
				if (torsion != 0)
				{
					for (int i = 1; i < list.Count; i++)
					{
						Movement movement2 = list[i].movement;
						if (movement2 == null || torsion != movement2.torsion)
						{
							torsion = 0;
							break;
						}
					}
				}
			}
		}
		int num;
		if (forcezerotorsion || e.type != EntType.sausage || e.direction.ParallelTo(dir))
		{
			num = 0;
		}
		else
		{
			num = -666;
		}
		Movement movement3 = Movement.Translation(e, dir, num, speed, Movement.MType.Idle, true);
		this.movements.Add(movement3);
		e.movement = movement3;
		if (!dir.Vertical())
		{
			this.PassiveForceSweep(false);
		}
		this.ApplyForce(e, dir, (dir != Direction.Up) ? 1 : 0, speed, true, weakforce);
		int torsion2 = movement3.torsion;
		this.movements.Remove(e.movement);
		e.movement = null;
		bool flag = false;
		if (!weakforce && e.type == EntType.sausage && !this.player.Extended())
		{
			if (dir.ParallelTo(e.direction))
			{
				Coord coord = ((dir != e.direction) ? (e.pos + dir) : (e.pos + 2 * dir.ToCoord()));
				if (this.fork.pos == coord && this.fork.movement == null && dir == this.fork.direction.Inverse() && this.fork.movement == null)
				{
					flag = true;
				}
			}
			else if ((this.fork.pos == e.pos + dir || this.fork.pos == e.pos + e.direction + dir) && dir == this.fork.direction.Inverse() && this.fork.movement == null)
			{
				if (this.Floor(this.player, false) == e)
				{
					this.RestoreEntities();
					Game.forktwang = true;
					return false;
				}
				flag = true;
				torsion = 0;
			}
		}
		this.AddTranslationOK(e, dir, num, speed);
		if (e.type == EntType.fork && !weakforce && e.stuckto == -1 && e.direction == dir)
		{
			Coord coord2 = e.TargetPos();
			List<Entity> list2 = this.EntsAt_NoAlloc(coord2, false, false);
			for (int j = 0; j < list2.Count; j++)
			{
				Entity entity = list2[j];
				if (entity.type == EntType.sausage && !this.ActivelyForced(entity) && list2.Count > 0)
				{
					e.stuckto = entity.id;
					entity.stuckto = e.id;
					this.forkfork = true;
					this.forksfx = true;
				}
			}
		}
		if (!e.CanMove(true, flag))
		{
			this.RestoreEntities();
			return false;
		}
		if ((e.type == EntType.island || e.movement.torsion == -666) && !dir.Vertical())
		{
			this.PassiveForceSweep(true);
		}
		if (torsion2 != e.movement.torsion)
		{
			this.RestoreEntities();
			return this.TryPushEnt(e, dir, torsion, speed, weakforce, true, canchangeplayerfooting, passive);
		}
		this.DiscardLastBackup();
		return true;
	}

	// Token: 0x06000A0F RID: 2575 RVA: 0x0002B980 File Offset: 0x00029D80
	private void SplashIsland(Entity e)
	{
		IntDictionary<List<Coord>> intDictionary = this.metagame.splashdat[e.dat];
		int num = -3 - e.pos.z;
		List<Coord> list;
		if (intDictionary.TryGetValue(num, out list))
		{
			foreach (Coord coord in list)
			{
				this.splashes.Add(coord + e.pos);
			}
		}
	}

	// Token: 0x06000A10 RID: 2576 RVA: 0x0002B9FC File Offset: 0x00029DFC
	public static string Merge(string s1, string s2)
	{
		if (s1.Length > 0 && s1[0] == 'I')
		{
			s1 = s1.Substring(s1.IndexOf('|'));
		}
		if (s2.Length > 0 && s2[0] == 'I')
		{
			s2 = s2.Substring(s2.IndexOf('|'));
		}
		MetaGameState metaGameState = MetaGameState.Blank();
		GameState gameState = ((s1.Length != 0) ? GameState.Load(s1, metaGameState, true) : GameState.Blank(metaGameState));
		GameState gameState2 = GameState.Load(s2, metaGameState, true);
		if (gameState2.entities.Count == 0)
		{
			return gameState.Save(false, false);
		}
		if (gameState.entities.Count == 0)
		{
			return gameState2.Save(false, false);
		}
		int num;
		if (s1.Length == 0)
		{
			num = -1;
		}
		else
		{
			num = gameState.entities.Max<Entity>((Entity e) => e.id);
		}
		int num2 = num;
		foreach (Entity entity in gameState2.entities)
		{
			entity.id += num2 + 1;
		}
		string text = gameState.Save(false, false);
		string text2 = gameState2.Save(false, false);
		return text + text2;
	}

	// Token: 0x06000A11 RID: 2577 RVA: 0x0002BB70 File Offset: 0x00029F70
	public void CalcBBQPositions(List<Coord> result)
	{
		result.Clear();
		foreach (Entity entity in this.entities)
		{
			EntType type = entity.type;
			if (type != EntType.bbq)
			{
				if (type == EntType.island)
				{
					entity.AppendIslandBBQPositions(result);
				}
			}
			else
			{
				result.Add(entity.pos);
			}
		}
	}

	// Token: 0x06000A12 RID: 2578 RVA: 0x0002BBE4 File Offset: 0x00029FE4
	public List<KeyValuePair<Coord, int>> TreePositions()
	{
		IEnumerable<Entity> enumerable = this.entities.Where<Entity>((Entity e) => e.type == EntType.ground && (e.tileset == 6 || e.tileset == 1) && e.tilenum == 6 && this.TreeColor(e.pos) > 0);
		return enumerable.Select<Entity, KeyValuePair<Coord, int>>((Entity e) => new KeyValuePair<Coord, int>(e.pos, this.TreeColor(e.pos))).ToList<KeyValuePair<Coord, int>>();
	}

	// Token: 0x06000A13 RID: 2579 RVA: 0x0002BC24 File Offset: 0x0002A024
	public void CalcCoast(List<Coord> result)
	{
		result.Clear();
		StringDictionary<IntDictionary<List<Coord>>> coastdat = this.metagame.coastdat;
		foreach (string text in coastdat.Keys)
		{
			IntDictionary<List<Coord>> intDictionary = coastdat[text];
			Entity entity = this.islandindex[text];
			if (entity != null)
			{
				int num = -entity.pos.z - 2;
				List<Coord> list;
				if (intDictionary.TryGetValue(num, out list))
				{
					for (int i = 0; i < list.Count; i++)
					{
						result.Add(list[i] + entity.pos);
					}
				}
			}
		}
	}

	// Token: 0x06000A14 RID: 2580 RVA: 0x0002BD0C File Offset: 0x0002A10C
	public List<Coord> Platforms()
	{
		List<Coord> list = new List<Coord>();
		foreach (Entity entity in this.entities)
		{
			if (!entity.type.Dynamic() && entity.type != EntType.island)
			{
				Coord coord = entity.pos + Direction.Up;
				List<Entity> list2 = this.EntsAt_NoAlloc(coord, false, false);
				if (!list2.Any<Entity>((Entity ent) => ent.type.Static()))
				{
					list.AddUnique(entity.pos);
				}
			}
		}
		return list;
	}

	// Token: 0x06000A15 RID: 2581 RVA: 0x0002BDB8 File Offset: 0x0002A1B8
	public static string Translate(string s, Coord offset)
	{
		GameState gameState = GameState.Load(s, null, true);
		if (gameState == null)
		{
			MetaGameState metaGameState = MetaGameState.Load(s, true);
			gameState = metaGameState.gamestate;
		}
		foreach (Entity entity in gameState.entities)
		{
			entity.pos += offset;
		}
		return gameState.Save(false, false);
	}

	// Token: 0x06000A16 RID: 2582 RVA: 0x0002BE48 File Offset: 0x0002A248
	public void TempIslandify(bool recalcmask)
	{
		if (this.dynamicentities.Any<Entity>((Entity ent) => ent.type == EntType.island))
		{
			return;
		}
		if (recalcmask)
		{
			IslandMask islandMask = this.LevelMask(Coord.Zero);
			this.metagame.splashdat.Add("temp", new IntDictionary<List<Coord>>());
			if (this.metagame.islandmasks.ContainsKey("temp"))
			{
				this.metagame.islandmasks["temp"] = islandMask;
			}
			else
			{
				this.metagame.islandmasks.Add("temp", islandMask);
			}
		}
		this.AddEntity(new Entity(this)
		{
			pos = new Coord(0, 0, 0),
			type = EntType.island,
			dat = "temp"
		}, false);
		for (int i = this.entities.Count - 1; i >= 0; i--)
		{
			bool flag = false;
			Entity entity = this.entities[i];
			if (entity.type == EntType.ground && entity.Decoration())
			{
				flag = true;
			}
			if (flag)
			{
				this.dynamicentities.Remove(entity);
				this.entities.RemoveAt(i);
				this.dynamicentityindex.Remove(entity.id);
				if (entity.type == EntType.island)
				{
					this.islandindex.Remove(entity.dat);
				}
			}
		}
	}

	// Token: 0x06000A17 RID: 2583 RVA: 0x0002BFC4 File Offset: 0x0002A3C4
	public static string NormalizeString(string s)
	{
		GameState gameState = GameState.Load(s, null, true);
		return gameState.Save(false, true);
	}

	// Token: 0x06000A18 RID: 2584 RVA: 0x0002BFE4 File Offset: 0x0002A3E4
	public void NormalizeState(bool justreorderents = false)
	{
		this.dynamicentities.Sort(delegate(Entity e0, Entity e1)
		{
			if (e0.pos.x < e1.pos.x)
			{
				return -1;
			}
			if (e0.pos.x > e1.pos.x)
			{
				return 1;
			}
			if (e0.pos.y < e1.pos.y)
			{
				return -1;
			}
			if (e0.pos.y > e1.pos.y)
			{
				return 1;
			}
			if (e0.pos.z < e1.pos.z)
			{
				return -1;
			}
			if (e0.pos.z > e1.pos.z)
			{
				return 1;
			}
			int num = e0.type.CompareTo(e1.type);
			if (num != 0)
			{
				return num;
			}
			return e0.dat.CompareTo(e1.dat);
		});
		this.entities.Sort(delegate(Entity e0, Entity e1)
		{
			if (e0.pos.x < e1.pos.x)
			{
				return -1;
			}
			if (e0.pos.x > e1.pos.x)
			{
				return 1;
			}
			if (e0.pos.y < e1.pos.y)
			{
				return -1;
			}
			if (e0.pos.y > e1.pos.y)
			{
				return 1;
			}
			if (e0.pos.z < e1.pos.z)
			{
				return -1;
			}
			if (e0.pos.z > e1.pos.z)
			{
				return 1;
			}
			int num2 = e0.type.CompareTo(e1.type);
			if (num2 != 0)
			{
				return num2;
			}
			return e0.dat.CompareTo(e1.dat);
		});
		this.UpdateDynamicIDDict();
		List<int> list = new List<int>();
		foreach (Entity entity in this.entities)
		{
			if (list.Contains(entity.id))
			{
				Debug.LogError("oops");
			}
			list.Add(entity.id);
			entity.id = list.Count - 1;
			if (entity.type == EntType.sausage)
			{
				entity.dat = string.Empty;
			}
		}
		foreach (Entity entity2 in this.entities)
		{
			if (entity2.stuckto >= 0)
			{
				entity2.stuckto = list.IndexOf(entity2.stuckto);
			}
		}
		this.SetIDCounterTo(list.Count);
		if (justreorderents)
		{
			return;
		}
		this.UpdateSMap();
		this.UpdateDynamicIDDict();
	}

	// Token: 0x06000A19 RID: 2585 RVA: 0x0002C164 File Offset: 0x0002A564
	public bool bbqsOn()
	{
		return (this.pushestotry == 0 && !this.overworld) || this.returning;
	}

	// Token: 0x06000A1A RID: 2586 RVA: 0x0002C188 File Offset: 0x0002A588
	private void DoCook(Entity e)
	{
		if (!this.bbqsOn())
		{
			return;
		}
		bool flag = false;
		if (e.type != EntType.sausage || e.pos.z <= -2)
		{
			return;
		}
		Coord pos = e.pos;
		Coord coord = e.pos + e.direction;
		string text;
		Direction direction = this.BBQAtDir(pos + Direction.Down, out text);
		string text2;
		Direction direction2 = this.BBQAtDir(coord + Direction.Down, out text2);
		if (direction == Direction.None && direction2 == Direction.None)
		{
			if (e.dat.Length == 0)
			{
				e.dat = "M;;";
			}
			else
			{
				char c = e.dat[0];
				if (c != 'L')
				{
					if (c != 'S')
					{
						if (c == 'M')
						{
							e.dat = "M;;";
						}
					}
					else
					{
						e.dat = "S;;";
					}
				}
				else
				{
					e.dat = "L;;";
				}
			}
			return;
		}
		int[] array = new int[]
		{
			e.cookdata % 4,
			e.cookdata / 4 % 4,
			e.cookdata / 16 % 4,
			e.cookdata / 64 % 4
		};
		string[] array2 = e.dat.Split(new char[] { ';' });
		if (array2.Length == 0 || array2[0].Length == 0)
		{
			array2 = new string[] { "M", " ", " " };
		}
		else if (array2.Length == 1)
		{
			array2 = new string[]
			{
				array2[0][0].ToString(),
				" ",
				" "
			};
		}
		if (direction != Direction.None && (array2.Length < 3 || array2[1] != text))
		{
			if (e.rot == 0)
			{
				if (array[3] == 0)
				{
					if (direction.ParallelTo(e.direction))
					{
						array[3] = 2;
					}
					else
					{
						array[3] = 1;
					}
				}
				else
				{
					array[3] = 3;
				}
				if (array[3] < 3)
				{
					this.sparks.Add(pos);
				}
				else
				{
					this.smokes.Add(pos);
				}
			}
			else
			{
				if (array[2] == 0)
				{
					if (direction.ParallelTo(e.direction))
					{
						array[2] = 2;
					}
					else
					{
						array[2] = 1;
					}
				}
				else
				{
					array[2] = 3;
				}
				if (array[2] < 3)
				{
					this.sparks.Add(pos);
				}
				else
				{
					this.smokes.Add(pos);
				}
			}
			flag = true;
		}
		if (direction2 != Direction.None && (array2.Length < 3 || array2[2] != text2))
		{
			if (e.rot == 0)
			{
				if (array[0] == 0)
				{
					if (direction2.ParallelTo(e.direction))
					{
						array[0] = 2;
					}
					else
					{
						array[0] = 1;
					}
				}
				else
				{
					array[0] = 3;
				}
				if (array[0] < 3)
				{
					this.sparks.Add(coord);
				}
				else
				{
					this.smokes.Add(coord);
				}
			}
			else
			{
				if (array[1] == 0)
				{
					if (direction2.ParallelTo(e.direction))
					{
						array[1] = 2;
					}
					else
					{
						array[1] = 1;
					}
				}
				else
				{
					array[1] = 3;
				}
				if (array[1] < 3)
				{
					this.sparks.Add(coord);
				}
				else
				{
					this.smokes.Add(coord);
				}
			}
			flag = true;
		}
		e.cookdata = array[0] + 4 * array[1] + 16 * array[2] + 64 * array[3];
		if (array[0] > 2 || array[1] > 2 || array[2] > 2 || array[3] > 2)
		{
			e.dat = "B;" + text + ";" + text2;
		}
		else if (e.dat.Length > 0)
		{
			e.dat = string.Concat(new object[]
			{
				e.dat[0],
				";",
				text,
				";",
				text2
			});
		}
		else
		{
			e.dat = "M;" + text + ";" + text2;
		}
		if (flag && this.overworld)
		{
			GameState.shoulddespawnstarts = true;
		}
	}

	// Token: 0x06000A1B RID: 2587 RVA: 0x0002C624 File Offset: 0x0002AA24
	public void TryLowerAll()
	{
		if (this.pushestotry <= 0)
		{
			return;
		}
		foreach (Entity entity in this.entities)
		{
			if (entity.type == EntType.island && entity.dat != this.pushtargetlevel)
			{
				if (this.pushestotry == 22)
				{
					if (entity.id % 3 < 2)
					{
						continue;
					}
				}
				else if (this.pushestotry == 21)
				{
					if (entity.id % 3 < 1)
					{
						continue;
					}
				}
				else if (this.pushestotry == 2)
				{
					if (entity.id % 3 >= 2)
					{
						continue;
					}
				}
				else if (this.pushestotry == 1 && entity.id % 3 >= 1)
				{
					continue;
				}
				this.TryPushEnt(entity, Direction.Down, 0, 1, false, false, false, false);
			}
		}
		this.ProcessGravity(false);
		if (this.movements.Count == 0)
		{
			this.pushestotry = 0;
		}
		else
		{
			this.pushestotry--;
		}
		if (this.pushestotry == 0)
		{
			this.donelowering = -1;
		}
	}

	// Token: 0x06000A1C RID: 2588 RVA: 0x0002C768 File Offset: 0x0002AB68
	public void TryRaiseAll()
	{
		if (this.pushestotry >= 0)
		{
			return;
		}
		for (int i = 0; i < this.entities.Count; i++)
		{
			Entity entity = this.entities[i];
			if (entity.type == EntType.island && entity.dat != this.pushtargetlevel && entity.pos.z < 0 && entity.Bottom() < -2)
			{
				if (this.movements.Count > 0)
				{
					if (this.pushestotry == -22)
					{
						if (entity.id % 3 < 2)
						{
							goto IL_00C7;
						}
					}
					else if (this.pushestotry == -21 && entity.id % 3 < 1)
					{
						goto IL_00C7;
					}
				}
				this.TryPushEnt(entity, Direction.Up, 0, 1, false, false, false, false);
			}
			IL_00C7:;
		}
		if (this.movements.Count == 0)
		{
			this.pushestotry = 0;
		}
		else
		{
			this.pushestotry++;
			if (this.pushestotry == 0)
			{
				this.pushestotry = -1;
			}
		}
		if (this.pushestotry == 0)
		{
			this.donelowering = 1;
		}
	}

	// Token: 0x06000A1D RID: 2589 RVA: 0x0002C8A0 File Offset: 0x0002ACA0
	public bool ApplyForce(Entity e, Direction dir, int torsion, int speed, bool recurse = true, bool weakforce = false)
	{
		bool flag = false;
		Coord[] array = e.Border(dir, true);
		if (this.ApplyForce(e.RoughOccupancyBounds_Wide(), array, dir, torsion, speed, weakforce, e))
		{
			flag = true;
		}
		return flag;
	}

	// Token: 0x06000A1E RID: 2590 RVA: 0x0002C8D4 File Offset: 0x0002ACD4
	public bool ApplyForce(Coord pos, Direction dir, int torsion, int speed, bool weakforce = false, Entity froment = null, bool canchangeplayerfooting = false)
	{
		List<Entity> list = this.EntsAt_Alloc(pos, false, false);
		foreach (Entity entity in list)
		{
			if ((entity.type != EntType.fork || !entity.Laden()) && entity.movement == null && !entity.Decoration())
			{
				Entity entity2 = entity.LadenTarget();
				this.EntsAt_Dealloc();
				return (entity2 == null || entity.LadenTarget().type != EntType.player) && this.TryPushEnt(entity, dir, torsion, speed, weakforce, false, canchangeplayerfooting, false);
			}
		}
		this.EntsAt_Dealloc();
		return false;
	}

	// Token: 0x06000A1F RID: 2591 RVA: 0x0002C978 File Offset: 0x0002AD78
	public bool ApplyForce(Coord pos, Direction dir, int torsion, int speed, out bool entsfound, bool weakforce = false, Entity froment = null)
	{
		List<Entity> list = this.EntsAt_Alloc(pos, false, false);
		entsfound = false;
		foreach (Entity entity in list)
		{
			if (entity != froment && (entity.type != EntType.fork || !entity.Laden()) && entity.movement == null && !entity.Decoration())
			{
				entsfound = true;
				Entity entity2 = entity.LadenTarget();
				this.EntsAt_Dealloc();
				return (entity2 == null || entity.LadenTarget().type != EntType.player) && this.TryPushEnt(entity, dir, torsion, speed, weakforce, false, false, false);
			}
		}
		this.EntsAt_Dealloc();
		return false;
	}

	// Token: 0x06000A20 RID: 2592 RVA: 0x0002CA2C File Offset: 0x0002AE2C
	public bool ApplyForce(BoundingBox bbox, Coord[] pos, Direction dir, int torsion, int speed, bool weakforce = false, Entity froment = null)
	{
		bool flag = true;
		this._boxneighbours_af_index++;
		if (this._boxneighbours_af_index >= GameState._boxneighbours_af.Count)
		{
			GameState._boxneighbours_af.Add(new List<HashSet<Entity>>());
		}
		List<HashSet<Entity>> list = GameState._boxneighbours_af[this._boxneighbours_af_index];
		this.CalcBoxNeighbours(bbox, list);
		foreach (HashSet<Entity> hashSet in list)
		{
			foreach (Entity entity in hashSet)
			{
				if (entity.movement == null && entity != froment && entity.At(pos, true, false) && (entity.type != EntType.fork || !entity.Laden()) && (entity.type != EntType.player || dir.Vertical()))
				{
					Entity entity2 = entity.LadenTarget();
					if (entity2 != null && entity2.type == EntType.player)
					{
						if (entity2.dat.Length == 0 && dir == Direction.Down)
						{
							if (!this.TryPushEnt(entity2, dir, torsion, speed, weakforce, false, false, false))
							{
								flag = false;
							}
							else
							{
								entity2.movement.mtype = Movement.MType.Fall;
							}
						}
					}
					else if (!this.TryPushEnt(entity, dir, torsion, speed, weakforce, false, false, false))
					{
						flag = false;
					}
				}
			}
		}
		this._boxneighbours_af_index--;
		return flag;
	}

	// Token: 0x06000A21 RID: 2593 RVA: 0x0002CBB4 File Offset: 0x0002AFB4
	public bool StableGround(Coord c)
	{
		Entity entity = this.EntAt(c + Coord.Down, false, false);
		return entity == null || entity.Decoration() || entity.moving;
	}

	// Token: 0x06000A22 RID: 2594 RVA: 0x0002CBF0 File Offset: 0x0002AFF0
	public bool Floating(Coord c)
	{
		Entity entity = this.EntAt(c + Coord.Down, false, false);
		return entity == null || entity.Decoration();
	}

	// Token: 0x06000A23 RID: 2595 RVA: 0x0002CC20 File Offset: 0x0002B020
	public Entity Footing(Entity e)
	{
		return this.EntAt(e.pos + Coord.Down, false, false);
	}

	// Token: 0x06000A24 RID: 2596 RVA: 0x0002CC3A File Offset: 0x0002B03A
	public bool HasFooting(Entity e)
	{
		return this.IsEntAt(e.pos + Coord.Down, false);
	}

	// Token: 0x06000A25 RID: 2597 RVA: 0x0002CC53 File Offset: 0x0002B053
	public bool Floating(Entity e)
	{
		return e.CouldMove(Direction.Down, 1, true);
	}

	// Token: 0x06000A26 RID: 2598 RVA: 0x0002CC5F File Offset: 0x0002B05F
	public Entity Floor(Coord c, bool instant = false)
	{
		return this.EntAt(c + Coord.Down, instant, true);
	}

	// Token: 0x06000A27 RID: 2599 RVA: 0x0002CC74 File Offset: 0x0002B074
	public Entity Floor(Entity e, bool instant = false)
	{
		return this.Floor(e.pos, instant);
	}

	// Token: 0x06000A28 RID: 2600 RVA: 0x0002CC84 File Offset: 0x0002B084
	public List<Entity> Under(Entity e, bool instant = false)
	{
		GameState._underents.Clear();
		Entity entity = this.Floor(e.pos, instant);
		if (entity != null)
		{
			GameState._underents.Add(entity);
		}
		if (e.Extended())
		{
			Entity entity2 = this.Floor(e.pos + e.direction, instant);
			if (entity2 != null && entity2 != entity)
			{
				GameState._underents.Add(entity2);
			}
		}
		return GameState._underents;
	}

	// Token: 0x06000A29 RID: 2601 RVA: 0x0002CCFC File Offset: 0x0002B0FC
	private bool ProcessGravity(bool tryreattachfork = true)
	{
		if (tryreattachfork)
		{
			this.TryReattachFork();
		}
		this.fallers.Clear();
		foreach (Entity entity in this.dynamicentities)
		{
			if (this.CanFall_Liberal(entity, true))
			{
				this.AddTranslation(entity, Direction.Down, 0, 1, (entity.type != EntType.player) ? Movement.MType.Idle : Movement.MType.Fall, true, true);
				this.fallers.Add(entity);
			}
		}
		bool flag = true;
		while (flag)
		{
			flag = false;
			for (int i = this.fallers.Count - 1; i >= 0; i--)
			{
				Entity entity2 = this.fallers[i];
				if (entity2.Collides(true, false))
				{
					this.movements.Remove(entity2.movement);
					entity2.movement = null;
					this.fallers.RemoveAt(i);
					flag = true;
					if (entity2.Laden())
					{
						Entity entity3 = entity2.LadenTarget();
						this.movements.Remove(entity3.movement);
						entity3.movement = null;
						int num = this.fallers.IndexOf(entity3);
						if (num >= 0)
						{
							this.fallers.RemoveAt(num);
							if (num < i)
							{
								i--;
							}
						}
					}
				}
			}
		}
		foreach (Entity entity4 in this.fallers)
		{
			if (entity4.type != EntType.island)
			{
				if (entity4.pos.z == -1)
				{
					this.splashes.Add(entity4.pos);
					this.splashes.Add(entity4.pos + entity4.direction);
					if (entity4.type == EntType.player)
					{
						this.drownsfx = true;
					}
				}
			}
		}
		return this.fallers.Count > 0;
	}

	// Token: 0x06000A2A RID: 2602 RVA: 0x0002CEF0 File Offset: 0x0002B2F0
	public void ContinueAutomatic()
	{
		bool flag = this.player.movement != null && this.player.dat.Length > 0;
		this.MovementsTick();
		if (this.pushestotry == 0 && this.wasraising)
		{
			this.IssueWorldSausages();
			if (Game.loadedLevelName == "WorldExplore")
			{
				string text = this.Save(true, false);
				SaveGame.SaveToSlot(text, this.sausagescooked, GameState.lastpushed);
			}
			this.wasraising = false;
		}
		if (this.pushestotry < 0)
		{
			this.wasraising = true;
		}
		if (this.pushestotry > 0)
		{
			this.waslowering = true;
		}
		if (!this.player.Turning())
		{
			bool flag2 = false;
			if (flag && this.player.movement == null)
			{
				flag2 = true;
				this.player.movement = Movement.Surprise(this.player, Direction.None, Movement.MType.TurnOut);
				this.movements.Add(this.player.movement);
			}
			this.ProcessGravity(true);
			if (flag && flag2)
			{
				this.movements.Remove(this.player.movement);
				this.player.movement = null;
			}
		}
		bool flag3 = false;
		foreach (Movement movement in this.movements)
		{
			if (movement.direction == Direction.Down)
			{
				flag3 = true;
				break;
			}
		}
		if (!flag3)
		{
			this.AutomaticPlayerTick(flag);
		}
		if (this.pushestotry == 0)
		{
			this.PassiveForceSweep(true);
		}
		this.ProcessGravity(true);
		this.TryDetatchFork();
		if (!this.Moving() && this.player.Extended())
		{
			Entity entity = this.Floor(this.player.pos, true);
			if (entity != null && entity.Pushable())
			{
				this.BakEntities();
				int num = this.splashes.Count<Coord>();
				this.SpawnFork();
				this.ProcessGravity(false);
				if (this.player.moving)
				{
					this.DiscardLastBackup();
				}
				else
				{
					this.RestoreEntities();
					this.splashes.RemoveRange(num, this.splashes.Count - num);
				}
			}
		}
		if (this.movements.Count == 0)
		{
			if (this.overworld)
			{
				this.CheckOverworldGhosts();
				this.CheckGameWon();
			}
			else
			{
				this.CheckOnLevelExit();
			}
		}
		this.ProcessPetalStuff();
	}

	// Token: 0x06000A2B RID: 2603 RVA: 0x0002D174 File Offset: 0x0002B574
	private void TryDetatchFork()
	{
		if (this.player == null || !this.player.Extended())
		{
			return;
		}
		foreach (Movement movement in this.movements)
		{
			if (movement.direction.Horizontal())
			{
				return;
			}
		}
		if (this.Falling(this.player) && this.player.pos.z >= -2)
		{
			this.SpawnFork();
			this.ProcessGravity(false);
		}
	}

	// Token: 0x06000A2C RID: 2604 RVA: 0x0002D20C File Offset: 0x0002B60C
	private void SpawnFork()
	{
		this.fork = new Entity(this);
		this.fork.pos = this.player.pos + this.player.direction;
		this.fork.direction = this.player.direction;
		this.fork.type = EntType.fork;
		Entity entity = this.player.LadenTarget();
		this.player.dat = string.Empty;
		this.player.cookdata = 1;
		this.player.Occupancy();
		bool flag = this.entitiesspawned;
		this.AddEntity(this.fork, false);
		if (entity != null)
		{
			entity.stuckto = this.fork.id;
			this.fork.stuckto = entity.id;
			this.player.stuckto = -1;
			this.forkfork = true;
		}
		this.addfork = true;
		this.entitiesspawned = flag;
		this.regenfork = true;
		this.detatchforksfx = true;
	}

	// Token: 0x06000A2D RID: 2605 RVA: 0x0002D30C File Offset: 0x0002B70C
	private void TryReattachFork()
	{
		if (this.player == null || this.player.Extended() || this.Falling(this.player) || this.player.moving)
		{
			return;
		}
		if (this.fork == null || this.fork.pos != this.player.pos + this.player.direction)
		{
			return;
		}
		if (this.fork.direction == this.player.direction && this.fork.movement == null)
		{
			if (this.fork.Laden())
			{
				this.player.stuckto = this.fork.stuckto;
				this.fork.LadenTarget().stuckto = this.player.id;
				this.forkfork = true;
			}
			bool flag = this.entitiesspawned;
			this.RemoveEntity(this.fork);
			this.entitiesspawned = flag;
			this.regenfork = true;
			this.fork = null;
			this.player.cookdata = 0;
			this.player.Occupancy();
			this.attachforksfx = true;
			if (this.player.dat.Length > 0)
			{
				int num = this.player.dat.IntParseFast();
				if (num < 0)
				{
					this.player.dat = (-9).ToStringFast();
				}
			}
		}
	}

	// Token: 0x06000A2E RID: 2606 RVA: 0x0002D494 File Offset: 0x0002B894
	private GameState.PassiveForce MergeForces(GameState.PassiveForce a, GameState.PassiveForce b)
	{
		if (a.direction != b.direction)
		{
			return new GameState.PassiveForce(Direction.None, 1, 0);
		}
		if (a.direction == Direction.None)
		{
			return new GameState.PassiveForce(Direction.None, Math.Max(a.speed, b.speed), 0);
		}
		int num = Math.Min(a.speed, b.speed);
		int num2;
		if (a.torsion != b.torsion)
		{
			num2 = 0;
		}
		else
		{
			num2 = a.torsion;
		}
		return new GameState.PassiveForce(a.direction, num, num2);
	}

	// Token: 0x06000A2F RID: 2607 RVA: 0x0002D52C File Offset: 0x0002B92C
	private List<Entity> FootprintEnts(Entity e)
	{
		GameState._footprintents.Clear();
		Coord[] array = e.SourceFootprintLower();
		this.CalcBoxNeighbours(e, GameState._footprintents_neighbours);
		if (this.curtowerlevel > 0)
		{
			BoundingBox boundingBox = e.RoughOccupancyBounds_Wide();
			bool flag = false;
			for (int i = 0; i < GameState._footprintents_neighbours.Count; i++)
			{
				HashSet<Entity> hashSet = GameState._footprintents_neighbours[i];
				foreach (Entity entity in hashSet)
				{
					if (entity.movement != null && boundingBox.Overlaps(entity.RoughOccupancyBounds_Wide()) && entity.At(array, true, true))
					{
						flag = true;
						break;
					}
				}
				if (flag)
				{
					break;
				}
			}
			if (!flag)
			{
				return GameState._footprintents;
			}
			BoundingBox boundingBox2 = this.bboxes[e.id];
			for (int j = 0; j < GameState._footprintents_neighbours.Count; j++)
			{
				HashSet<Entity> hashSet2 = GameState._footprintents_neighbours[j];
				foreach (Entity entity2 in hashSet2)
				{
					if (this.bboxes[entity2.id].Overlaps(boundingBox2) && entity2.At(array, true, true))
					{
						if (entity2.movement == null || !entity2.movement.direction.Vertical())
						{
							GameState._footprintents.Add(entity2);
						}
					}
				}
			}
		}
		else
		{
			for (int k = 0; k < GameState._footprintents_neighbours.Count; k++)
			{
				HashSet<Entity> hashSet3 = GameState._footprintents_neighbours[k];
				foreach (Entity entity3 in hashSet3)
				{
					if (entity3.At(array, true, true))
					{
						GameState._footprintents.Add(entity3);
					}
				}
			}
		}
		return GameState._footprintents;
	}

	// Token: 0x06000A30 RID: 2608 RVA: 0x0002D740 File Offset: 0x0002BB40
	private int GetPassiveSpeed(Entity e)
	{
		if (e == this.player)
		{
			return 0;
		}
		List<Entity> list = this.FootprintEnts(e);
		int count = list.Count;
		bool flag = this.ConsistentFootprint(list);
		if (!flag)
		{
			return 0;
		}
		GameState.PassiveForce passiveForce = this.ExtractPassiveForce(list[0]);
		for (int i = 1; i < count; i++)
		{
			passiveForce = this.MergeForces(passiveForce, this.ExtractPassiveForce(list[i]));
		}
		if (passiveForce.direction.NormalTo(e.direction) || passiveForce.torsion == 0)
		{
			return passiveForce.speed;
		}
		return passiveForce.speed + 1;
	}

	// Token: 0x06000A31 RID: 2609 RVA: 0x0002D7E8 File Offset: 0x0002BBE8
	private bool ApplyPassiveForce(Entity e)
	{
		if (e == this.player)
		{
			return false;
		}
		bool flag = false;
		Coord[] array = e.SourceFootprintLower();
		BoundingBox boundingBox = e.RoughOccupancyBounds_Wide();
		List<HashSet<Entity>> list = GameState.boxneighbours_tower[this.curtowerlevel];
		this.CalcBoxNeighbours(boundingBox, list);
		bool flag2 = false;
		for (int i = 0; i < list.Count; i++)
		{
			HashSet<Entity> hashSet = list[i];
			foreach (Entity entity in hashSet)
			{
				if (entity.movement != null && entity.movement.translation && !entity.movement.direction.Vertical() && entity.At(array, true, true))
				{
					flag2 = true;
					break;
				}
			}
			if (flag2)
			{
				break;
			}
		}
		if (!flag2)
		{
			return false;
		}
		List<Entity> list2 = GameState._applyforce_footprints_ents[this.curtowerlevel];
		list2.Clear();
		for (int j = 0; j < list.Count; j++)
		{
			HashSet<Entity> hashSet2 = list[j];
			foreach (Entity entity2 in hashSet2)
			{
				if (boundingBox.Overlaps(entity2.RoughOccupancyBounds_Wide()) && entity2.At(array, true, true))
				{
					if (entity2.movement == null || entity2.movement.direction != Direction.Down)
					{
						list2.Add(entity2);
					}
				}
			}
		}
		bool flag3 = true;
		if (list2.Count == 0)
		{
			return false;
		}
		bool flag4 = true;
		bool flag5 = true;
		foreach (Entity entity3 in list2)
		{
			if (entity3.movement == null)
			{
				flag5 = false;
			}
			else
			{
				flag4 = false;
			}
		}
		if (flag4)
		{
			return false;
		}
		if (flag5)
		{
			flag3 = false;
		}
		if (flag3)
		{
			this.BakEntities();
		}
		else if (!this.ConsistentFootprint(list2))
		{
			return false;
		}
		bool flag6 = false;
		GameState.PassiveForce passiveForce = default(GameState.PassiveForce);
		for (int k = 0; k < list2.Count; k++)
		{
			if (list2[k].movement != null)
			{
				GameState.PassiveForce passiveForce2 = this.ExtractPassiveForce(list2[k]);
				if (flag6)
				{
					passiveForce = this.MergeForces(passiveForce, passiveForce2);
				}
				else
				{
					passiveForce = passiveForce2;
					flag6 = true;
				}
			}
		}
		if (!flag6)
		{
			passiveForce = new GameState.PassiveForce(Direction.None, 0, 0);
		}
		List<int> list3 = new List<int>();
		foreach (Movement movement in this.movements)
		{
			if (movement.target.type == EntType.island && movement.towerlevel == this.curtowerlevel)
			{
				list3.Add(movement.targetid);
			}
		}
		if (passiveForce.direction != Direction.None)
		{
			if ((e.Extended() && passiveForce.direction.NormalTo(e.direction)) || passiveForce.torsion == 0)
			{
				int speed = passiveForce.speed;
				if ((!e.moving || speed >= e.movement.speed) && this.TryPushEnt(e, passiveForce.direction, -1 * passiveForce.torsion, speed, true, false, false, false))
				{
					flag = true;
				}
			}
			else if (passiveForce.torsion != -1)
			{
				bool flag7 = false;
				int num = passiveForce.speed + 1;
				if (!e.moving || num > e.movement.speed)
				{
					flag7 = this.TryPushEnt(e, passiveForce.direction, 0, num, false, false, false, false);
				}
				if (flag7)
				{
					if (this.ConsistentFootprint(list2))
					{
						flag = true;
					}
					else
					{
						flag7 = false;
						this.RestoreEntities();
						this.BakEntities();
					}
				}
				if (!flag7)
				{
					int speed2 = passiveForce.speed;
					if ((!e.moving || speed2 > e.movement.speed) && this.TryPushEnt(e, passiveForce.direction, 0, speed2, false, false, false, false))
					{
						flag = true;
					}
				}
			}
			if (flag)
			{
				foreach (Movement movement2 in this.movements)
				{
					if (movement2.target.type == EntType.island && movement2.mtype != Movement.MType.Fixed && !this.ConsistentFootprint(this.FootprintEnts(movement2.target)))
					{
						if (flag3)
						{
							this.RestoreEntities();
						}
						return false;
					}
				}
			}
		}
		if (!flag3)
		{
			return flag;
		}
		if (!flag)
		{
			this.DiscardLastBackup();
			return false;
		}
		bool flag8 = this.ConsistentFootprint(list2);
		if (flag8)
		{
			this.DiscardLastBackup();
			bool flag9 = this.movements.Any<Movement>((Movement m) => m.target.type == EntType.island && m.direction.Horizontal());
			if (flag9)
			{
				this.PassiveForceSweep(true);
			}
			return true;
		}
		this.RestoreEntities();
		return false;
	}

	// Token: 0x06000A32 RID: 2610 RVA: 0x0002DD54 File Offset: 0x0002C154
	private bool ConsistentFootprint(List<Entity> footprint_ents)
	{
		if (footprint_ents.Count == 0)
		{
			return false;
		}
		foreach (Entity entity in footprint_ents)
		{
			if (entity.movement == null || !entity.movement.translation || entity.movement.direction == Direction.None)
			{
				return false;
			}
		}
		return true;
	}

	// Token: 0x06000A33 RID: 2611 RVA: 0x0002DDC0 File Offset: 0x0002C1C0
	private GameState.PassiveForce ExtractPassiveForce(Entity forcer)
	{
		if (!forcer.moving)
		{
			return new GameState.PassiveForce(Direction.None, 0, 0);
		}
		Movement movement = forcer.movement;
		if (movement.movetype == Movement.MoveType.Rotation || movement.movetype == Movement.MoveType.None)
		{
			return new GameState.PassiveForce(Direction.None, 0, 0);
		}
		if (movement.torsion == -666)
		{
			this.CalculateTorsion(movement.target);
		}
		return new GameState.PassiveForce(movement.direction, movement.speed, movement.torsion);
	}

	// Token: 0x06000A34 RID: 2612 RVA: 0x0002DE3C File Offset: 0x0002C23C
	private void CalculateTorsion(Entity e)
	{
		Movement movement = e.movement;
		if (e.type == EntType.island)
		{
			movement.torsion = 0;
			return;
		}
		if (e.direction.ParallelTo(movement.direction) || !e.type.CanRoll() || movement.direction.Vertical())
		{
			movement.torsion = 0;
			return;
		}
		if (e.type == EntType.sausage && movement.direction.Valid())
		{
			Coord coord = e.pos + movement.direction;
			bool flag = this.fork != null && this.fork.movement == null && (this.fork.pos == coord || this.fork.pos == coord + e.direction) && this.fork.direction == movement.direction.Inverse();
			if (flag)
			{
				this.BakEntities();
				this.nopassivesweeps = true;
				bool flag2 = this.TryPushEnt(this.fork, movement.direction, 0, movement.speed, false, false, false, false);
				this.nopassivesweeps = false;
				this.RestoreEntities();
				if (!flag2)
				{
					movement.torsion = 0;
					return;
				}
			}
		}
		Coord[] array = e.SourceFootprintLower();
		BoundingBox boundingBox = e.RoughOccupancyBounds_Wide();
		List<HashSet<Entity>> list = this.BoxNeighbours(new BoundingBox(array));
		GameState._footprint_ents.Clear();
		for (int i = 0; i < list.Count; i++)
		{
			HashSet<Entity> hashSet = list[i];
			foreach (Entity entity in hashSet)
			{
				if (boundingBox.Overlaps(entity.RoughOccupancyBounds_Wide()) && entity.At(array, true, true))
				{
					if (entity.movement == null || !entity.movement.direction.Vertical())
					{
						GameState._footprint_ents.Add(entity);
					}
				}
			}
		}
		if (GameState._footprint_ents.Count == 0)
		{
			movement.torsion = 0;
			return;
		}
		GameState.PassiveForce passiveForce = this.ExtractPassiveForce(GameState._footprint_ents[0]);
		for (int j = 1; j < GameState._footprint_ents.Count; j++)
		{
			GameState.PassiveForce passiveForce2 = this.ExtractPassiveForce(GameState._footprint_ents[j]);
			passiveForce = this.MergeForces(passiveForce, passiveForce2);
		}
		if (passiveForce.direction.Vertical() || (passiveForce.direction.Invalid() && passiveForce.speed > 0))
		{
			movement.torsion = 0;
			return;
		}
		if (passiveForce.torsion != 0)
		{
			Occupancy[] array2 = e.Occupancy();
			for (int k = 0; k < array2.Length; k++)
			{
				Occupancy[] array3 = array2;
				int num = k;
				array3[num].pos = array3[num].pos + Direction.Down;
			}
			List<Entity> list2 = new List<Entity>();
			for (int l = 0; l < GameState._footprint_ents.Count; l++)
			{
				Entity entity2 = GameState._footprint_ents[l];
				Occupancy[] array4 = entity2.Occupancy();
				bool flag3 = false;
				foreach (Occupancy occupancy in array4)
				{
					foreach (Occupancy occupancy2 in array2)
					{
						if (occupancy.Overlaps(occupancy2))
						{
							flag3 = true;
							break;
						}
					}
					if (flag3)
					{
						break;
					}
				}
				if (flag3)
				{
					list2.Add(entity2);
				}
			}
			if (list2.Count == 0)
			{
				movement.torsion = 0;
			}
			else
			{
				int num2 = list2[0].movement.speed - list2[0].movement.torsion;
				for (int num3 = 1; num3 < list2.Count; num3++)
				{
					int num4 = list2[num3].movement.speed - list2[num3].movement.torsion;
					if (num2 != num4)
					{
						num2 = 0;
						break;
					}
				}
				int num5;
				if (movement.speed > num2 + 1)
				{
					num5 = 1;
				}
				else if (movement.speed == num2 + 1)
				{
					num5 = -1;
				}
				else if (movement.speed == num2 - 1)
				{
					num5 = 1;
				}
				else
				{
					num5 = 0;
				}
				movement.torsion = num5;
			}
		}
		else if (movement.speed == passiveForce.speed)
		{
			movement.torsion = 0;
		}
		else if (movement.speed < passiveForce.speed)
		{
			movement.torsion = -1;
		}
		else
		{
			movement.torsion = 1;
		}
	}

	// Token: 0x06000A35 RID: 2613 RVA: 0x0002E330 File Offset: 0x0002C730
	private void CalculateTorsions(bool applytorque)
	{
		for (int i = 0; i < this.movements.Count; i++)
		{
			Movement movement = this.movements[i];
			if (movement.torsion == -666)
			{
				this.CalculateTorsion(movement.target);
				Entity target = movement.target;
				if (applytorque && movement.torsion != 0)
				{
					target.TryRotate(movement.direction);
				}
				if (movement.torsion != 0 && movement.torsion != -666 && movement.direction.Horizontal())
				{
					this.anyrolls = Mathf.Max(this.anyrolls, this.GetSurfaceType(target.pos + Coord.Down));
					this.anyrolls = Mathf.Max(this.anyrolls, this.GetSurfaceType(target.pos + target.direction + Coord.Down));
				}
				else if (target.type == EntType.sausage && movement.direction.Horizontal())
				{
					this.anydrags = Mathf.Max(this.anydrags, this.GetSurfaceType(target.pos + Coord.Down));
					this.anydrags = Mathf.Max(this.anydrags, this.GetSurfaceType(target.pos + target.direction + Coord.Down));
				}
			}
		}
	}

	// Token: 0x06000A36 RID: 2614 RVA: 0x0002E4A4 File Offset: 0x0002C8A4
	public List<HashSet<Entity>> BoxNeighbours(Entity e)
	{
		return this.BoxNeighbours(e.RoughOccupancyBounds_Wide());
	}

	// Token: 0x06000A37 RID: 2615 RVA: 0x0002E4B2 File Offset: 0x0002C8B2
	public void CalcBoxNeighbours(Entity e, List<HashSet<Entity>> result)
	{
		this.CalcBoxNeighbours(e.RoughOccupancyBounds_Wide(), result);
	}

	// Token: 0x06000A38 RID: 2616 RVA: 0x0002E4C4 File Offset: 0x0002C8C4
	public void CalcBoxNeighbours(BoundingBox bbox, List<HashSet<Entity>> result)
	{
		result.Clear();
		int num = bbox.min.x / 10;
		int num2 = bbox.max.x / 10;
		int num3 = bbox.min.y / 10;
		int num4 = bbox.max.y / 10;
		for (int i = num; i <= num2; i++)
		{
			for (int j = num3; j <= num4; j++)
			{
				int num5 = i * 10000 + j;
				HashSet<Entity> hashSet;
				if (this.boxes.TryGetValue(num5, out hashSet))
				{
					result.Add(hashSet);
				}
			}
		}
	}

	// Token: 0x06000A39 RID: 2617 RVA: 0x0002E580 File Offset: 0x0002C980
	public List<HashSet<Entity>> BoxNeighbours(BoundingBox bbox)
	{
		int num = bbox.min.x / 10;
		int num2 = bbox.max.x / 10;
		int num3 = bbox.min.y / 10;
		int num4 = bbox.max.y / 10;
		List<HashSet<Entity>> list = new List<HashSet<Entity>>((num2 - num + 1) * (num4 - num3 + 1));
		for (int i = num; i <= num2; i++)
		{
			for (int j = num3; j <= num4; j++)
			{
				int num5 = i * 10000 + j;
				HashSet<Entity> hashSet;
				if (this.boxes.TryGetValue(num5, out hashSet))
				{
					list.Add(hashSet);
				}
			}
		}
		return list;
	}

	// Token: 0x06000A3A RID: 2618 RVA: 0x0002E64C File Offset: 0x0002CA4C
	public HashSet<Entity> BoxAt(Coord c)
	{
		int num = c.x / 10;
		int num2 = c.y / 10;
		int num3 = num * 10000 + num2;
		HashSet<Entity> hashSet;
		this.boxes.TryGetValue(num3, out hashSet);
		return hashSet;
	}

	// Token: 0x06000A3B RID: 2619 RVA: 0x0002E68C File Offset: 0x0002CA8C
	public void RemoveBox(Entity e)
	{
		BoundingBox boundingBox = e.RoughOccupancyBounds_Wide();
		int num = boundingBox.min.x / 10;
		int num2 = boundingBox.max.x / 10;
		int num3 = boundingBox.min.y / 10;
		int num4 = boundingBox.max.y / 10;
		for (int i = num - 1; i <= num2 + 1; i++)
		{
			for (int j = num3 - 1; j <= num4 + 1; j++)
			{
				int num5 = i * 10000 + j;
				HashSet<Entity> hashSet;
				if (this.boxes.TryGetValue(num5, out hashSet))
				{
					hashSet.Remove(e);
					if (hashSet.Count == 0)
					{
						this.boxes.Remove(num5);
					}
				}
			}
		}
	}

	// Token: 0x06000A3C RID: 2620 RVA: 0x0002E76C File Offset: 0x0002CB6C
	public void BuildBox(Entity e)
	{
		e.CalcRoughOccupancyBounds();
		BoundingBox boundingBox = e.RoughOccupancyBounds_Wide();
		int num = boundingBox.min.x / 10;
		int num2 = boundingBox.max.x / 10;
		int num3 = boundingBox.min.y / 10;
		int num4 = boundingBox.max.y / 10;
		for (int i = num - 1; i <= num2 + 1; i++)
		{
			int num5 = i * 10000 + (num3 - 1);
			int num6 = i * 10000 + (num4 + 1);
			if (this.boxes.ContainsKey(num5) && this.boxes[num5].Contains(e))
			{
				this.boxes[num5].Remove(e);
			}
			if (this.boxes.ContainsKey(num6) && this.boxes[num6].Contains(e))
			{
				this.boxes[num6].Remove(e);
			}
		}
		for (int j = num3; j <= num4; j++)
		{
			int num7 = (num - 1) * 10000 + j;
			int num8 = (num2 + 1) * 10000 + j;
			if (this.boxes.ContainsKey(num7) && this.boxes[num7].Contains(e))
			{
				this.boxes[num7].Remove(e);
				if (this.boxes[num7].Count == 0)
				{
					this.boxes.Remove(num7);
				}
			}
			if (this.boxes.ContainsKey(num8) && this.boxes[num8].Contains(e))
			{
				this.boxes[num8].Remove(e);
				if (this.boxes[num8].Count == 0)
				{
					this.boxes.Remove(num8);
				}
			}
		}
		for (int k = num; k <= num2; k++)
		{
			for (int l = num3; l <= num4; l++)
			{
				int num9 = k * 10000 + l;
				HashSet<Entity> hashSet;
				if (this.boxes.TryGetValue(num9, out hashSet))
				{
					if (!hashSet.Contains(e))
					{
						hashSet.Add(e);
					}
				}
				else
				{
					this.boxes.Add(num9, new HashSet<Entity> { e });
				}
			}
		}
	}

	// Token: 0x06000A3D RID: 2621 RVA: 0x0002EA10 File Offset: 0x0002CE10
	public void BuildBoxes()
	{
		this.boxes.Clear();
		foreach (Entity entity in this.entities)
		{
			entity.CalcRoughOccupancyBounds();
			BoundingBox boundingBox = entity.RoughOccupancyBounds_Wide();
			int num = boundingBox.min.x / 10;
			int num2 = boundingBox.max.x / 10;
			int num3 = boundingBox.min.y / 10;
			int num4 = boundingBox.max.y / 10;
			for (int i = num; i <= num2; i++)
			{
				for (int j = num3; j <= num4; j++)
				{
					int num5 = i * 10000 + j;
					HashSet<Entity> hashSet;
					if (this.boxes.TryGetValue(num5, out hashSet))
					{
						hashSet.Add(entity);
					}
					else
					{
						this.boxes.Add(num5, new HashSet<Entity> { entity });
					}
				}
			}
		}
	}

	// Token: 0x06000A3E RID: 2622 RVA: 0x0002EB2C File Offset: 0x0002CF2C
	private void InsertionSort(List<Entity> array, Comparison<Entity> comparer)
	{
		for (int i = 1; i < array.Count; i++)
		{
			Entity entity = array[i];
			int num = i - 1;
			while (num >= 0 && comparer(array[num], entity) > 0)
			{
				array[num + 1] = array[num];
				num--;
			}
			array[num + 1] = entity;
		}
	}

	// Token: 0x06000A3F RID: 2623 RVA: 0x0002EB9C File Offset: 0x0002CF9C
	public List<Entity> OverWaterMovements()
	{
		this.overwatermovements.Clear();
		for (int i = 0; i < this.movements.Count<Movement>(); i++)
		{
			Movement movement = this.movements[i];
			Entity target = movement.target;
			if (target.moving)
			{
				if (target.RoughOccupancyBounds().max.z > -2)
				{
					this.overwatermovements.Add(target);
				}
			}
		}
		return this.overwatermovements;
	}

	// Token: 0x06000A40 RID: 2624 RVA: 0x0002EC24 File Offset: 0x0002D024
	public bool OnlySubmergedMovements()
	{
		for (int i = 0; i < this.movements.Count<Movement>(); i++)
		{
			Movement movement = this.movements[i];
			Entity target = movement.target;
			if (target.moving)
			{
				if (target.RoughOccupancyBounds().max.z > -2)
				{
					return false;
				}
			}
		}
		return true;
	}

	// Token: 0x06000A41 RID: 2625 RVA: 0x0002EC94 File Offset: 0x0002D094
	public void SortDynamicEnts()
	{
		GameState.cacheddat.Clear();
		for (int i = 0; i < this.dynamicentities.Count; i++)
		{
			Entity entity = this.dynamicentities[i];
			KeyValuePair<int, float> keyValuePair = new KeyValuePair<int, float>(this.GetPassiveSpeed(entity), (float)Entity.DistanceSqFromPlayer(entity, this.player));
			GameState.cacheddat[entity.id] = keyValuePair;
		}
		this.InsertionSort(this.dynamicentities, GameState._dynamicSortFn);
	}

	// Token: 0x06000A42 RID: 2626 RVA: 0x0002ED14 File Offset: 0x0002D114
	public void PassiveForceSweep(bool applytorque = true)
	{
		if (this.movements.Count == 0 || this.pushestotry != 0 || this.nopassivesweeps)
		{
			return;
		}
		this.curtowerlevel++;
		while (GameState.bboxes_tower.Count <= this.curtowerlevel + 1)
		{
			GameState.bboxes_tower.Add(new IntDictionary<BoundingBox>(this.entities.Count));
			GameState.boxneighbours_tower.Add(new List<HashSet<Entity>>(4));
			GameState._applyforce_footprints_ents.Add(new List<Entity>(4));
		}
		this.bboxes = GameState.bboxes_tower[this.curtowerlevel];
		this.bboxes.Clear();
		for (int i = 0; i < this.entities.Count; i++)
		{
			Entity entity = this.entities[i];
			this.bboxes[entity.id] = entity.RoughOccupancyBounds();
		}
		Direction direction = Direction.None;
		for (int j = 0; j < this.movements.Count; j++)
		{
			Movement movement = this.movements[j];
			Direction direction2 = movement.EffectiveDir();
			if (direction2.Horizontal() && direction2 != Direction.None)
			{
				direction = direction2;
				break;
			}
		}
		this.CalculateTorsions(applytorque);
		if (this.curtowerlevel == 1)
		{
			this.SortDynamicEnts();
		}
		bool flag = true;
		while (flag)
		{
			flag = false;
			for (int k = 0; k < this.dynamicentities.Count; k++)
			{
				Entity entity2 = this.dynamicentities[k];
				if (applytorque || entity2.type != EntType.sausage || entity2.direction.ParallelTo(direction))
				{
					if (entity2.movement == null)
					{
						Entity entity3 = entity2.LadenTarget();
						if (entity3 == null || !entity3.Extended())
						{
							if (this.ApplyPassiveForce(entity2))
							{
								this.CalculateTorsions(applytorque);
								flag = true;
							}
						}
					}
				}
			}
		}
		this.curtowerlevel--;
		this.bboxes = GameState.bboxes_tower[this.curtowerlevel];
	}

	// Token: 0x06000A43 RID: 2627 RVA: 0x0002EF50 File Offset: 0x0002D350
	private void AutomaticTurn(Entity e, int turnspeed = 2)
	{
		if (e.movement != null)
		{
			return;
		}
		Direction direction = e.direction;
		Direction turndir = e.turndir;
		Direction direction2 = DirectionUtil.ContinueRot(turndir, direction);
		this.BakEntities();
		if (e.Extended())
		{
			Entity entity = this.EntAt(e.pos + turndir, false, false);
			if (entity != null && this.TryPushEnt(entity, direction2.Inverse(), 1, 1, true, false, false, false))
			{
				turnspeed = 1;
			}
		}
		Coord coord = e.pos + turndir;
		Movement movement = Movement.Rotation(e, e.direction, turndir, Movement.MType.TurnOut, 1);
		e.movement = movement;
		this.movements.Add(e.movement);
		Entity entity2 = null;
		if (e == this.player)
		{
			entity2 = this.Floor(this.player, false);
			if (entity2 != null && !entity2.type.Static())
			{
				this.movements.Remove(entity2.movement);
				entity2.movement = Movement.Fixed(entity2, turnspeed);
				this.movements.Add(entity2.movement);
			}
		}
		Direction direction3 = direction2.Inverse();
		if (e.Extended())
		{
			bool flag;
			if (!this.ApplyForce(coord, direction3, 1, 1, out flag, false, null))
			{
				if (e == this.player)
				{
					List<Entity> list = this.EntsAt_NoAlloc(coord, true, false);
					foreach (Entity entity3 in list)
					{
						if (entity3 != this.player)
						{
							this.moveattempts[entity3.id] = direction3;
						}
					}
				}
			}
			if (flag)
			{
				turnspeed = 1;
			}
		}
		this.movements.Remove(e.movement);
		e.movement = null;
		if (entity2 != null && entity2.moving)
		{
			entity2.movement.SetSpeed(turnspeed);
		}
		this.AddRotation(e, e.direction, turndir, Movement.MType.TurnOut, turnspeed);
		if (e.Collides(true, false))
		{
			this.RestoreEntities();
			if (e == this.player)
			{
				if (!this.AutomaticPivotTurn(e, Direction.None))
				{
					this.moveattempts[this.idcounter] = direction3;
					this.RotateBack(e);
				}
			}
			else
			{
				this.RotateBack(e);
			}
		}
		else
		{
			this.DiscardLastBackup();
			Entity hat = this.GetHat(e);
			if (hat != null && hat.Turning())
			{
				this.AutomaticTurn(hat, turnspeed);
			}
		}
		e.turndir = Direction.None;
	}

	// Token: 0x06000A44 RID: 2628 RVA: 0x0002F1D0 File Offset: 0x0002D5D0
	private void RotateBack(Entity e)
	{
		Direction direction = e.direction;
		if (!direction.Diagonal())
		{
			return;
		}
		this.BakEntities();
		Direction turndir = e.turndir;
		Direction direction2 = DirectionUtil.ContinueRot(turndir, direction);
		Coord coord = e.pos + direction2;
		int num = 2;
		if (e.Extended())
		{
			Movement movement = Movement.Rotation(e, e.direction, direction2, Movement.MType.TurnBackout, 1);
			e.movement = movement;
			this.movements.Add(e.movement);
			if (this.ApplyForce(coord, turndir.Inverse(), 1, 1, false, null, false))
			{
				num = 1;
			}
			this.movements.Remove(e.movement);
			e.movement = null;
		}
		this.AddRotation(e, e.direction, direction2, Movement.MType.TurnBackout, num);
		Entity hat = this.GetHat(e);
		if (hat != null)
		{
			this.RotateBack(hat);
		}
		if (e.Collides(true, false))
		{
			this.RestoreEntities();
			Debug.LogError("dead end state");
		}
		else
		{
			this.DiscardLastBackup();
		}
	}

	// Token: 0x06000A45 RID: 2629 RVA: 0x0002F2D0 File Offset: 0x0002D6D0
	private Entity GetHat(Entity player)
	{
		Coord coord = player.pos + Direction.Up;
		Entity entity = this.EntAt(coord, false, true);
		if (entity == null || !entity.type.CanHatTurn())
		{
			return null;
		}
		if (entity.type == EntType.sausage && !entity.pos.Above(player.pos))
		{
			entity.Pivot();
		}
		if (this.Under(entity, false).Count == 1 || entity.direction.Diagonal())
		{
			return entity;
		}
		return null;
	}

	// Token: 0x06000A46 RID: 2630 RVA: 0x0002F35C File Offset: 0x0002D75C
	private void AutomaticClimbUp(Entity e)
	{
		this.BakEntities();
		int num = e.dat.IntParseFast();
		Direction direction = (Direction)num;
		Coord coord = e.pos + direction;
		bool flag = direction.LeftOf(e.direction);
		Entity entity;
		if (this.LadderAt(coord, out entity) == direction.Inverse())
		{
			Movement.MType mtype = ((this.LadderAt(coord + Direction.Up) != direction.Inverse()) ? Movement.MType.ClimbUp_End1 : Movement.MType.ClimbUp_Loop);
			this.AddTranslation(e, Direction.Up, 0, 1, mtype, flag, true);
			this.ApplyForce(e, Direction.Up, 0, 1, true, false);
			if (e.Collides(true, false) || entity.movement != null)
			{
				this.RestoreEntities();
				this.AddTranslation(e, Direction.Down, 0, 1, Movement.MType.ClimbDown_Loop, !flag, true);
				this.ApplyForce(e, Direction.Down, 0, 1, true, false);
				e.dat = ((int)((Direction)(-1) - direction.Inverse())).ToStringFast();
			}
			else
			{
				this.DiscardLastBackup();
			}
		}
		else
		{
			this.LadderAt(coord + Direction.Down, out entity);
			this.AddTranslation(e, direction, 0, 1, Movement.MType.ClimbUp_End2, flag, true);
			this.ApplyForce(e, direction, 0, 1, true, false);
			this.TryFork();
			e.dat = string.Empty;
			if (e.Collides(true, false) || (entity != null && entity.movement != null))
			{
				this.RestoreEntities();
				this.BakEntities();
				this.AddTranslation(e, Direction.Down, 0, 1, Movement.MType.ClimbDown_Init2, !flag, true);
				e.dat = ((int)((Direction)(-1) - direction.Inverse())).ToStringFast();
				if (e.Collides(true, false))
				{
					this.RestoreEntities();
					e.dat = string.Empty;
				}
				else
				{
					this.DiscardLastBackup();
				}
			}
			else
			{
				Coord coord2 = this.player.pos + direction + Coord.Down;
				this.playerclimbdownland = Mathf.Max(this.playerclimbdownland, this.GetSurfaceType(coord2));
				this.DiscardLastBackup();
			}
		}
	}

	// Token: 0x06000A47 RID: 2631 RVA: 0x0002F548 File Offset: 0x0002D948
	private void AutomaticClimbDown(Entity e)
	{
		this.BakEntities();
		int num = e.dat.IntParseFast();
		Direction direction = (Direction)(-num - 1);
		if (direction == Direction.None)
		{
			this.AddTranslation(e, e.direction, 0, 1, Movement.MType.ClimbUp_End2, true, true);
			this.ApplyForce(e, e.direction, 1, 1, true, false);
			if (e.Collides(true, false))
			{
				this.RestoreEntities();
				e.dat = string.Empty;
			}
			else
			{
				this.DiscardLastBackup();
				e.movement.mtype = Movement.MType.ForwardPedal;
				e.dat = string.Empty;
			}
			return;
		}
		bool flag = direction.LeftOf(e.direction);
		Entity entity;
		Movement.MType mtype = ((this.LadderAt(e.pos - direction, out entity) != direction) ? Movement.MType.ClimbDown_Init2 : Movement.MType.ClimbDown_Loop);
		this.AddTranslation(e, Direction.Down, 0, 1, mtype, flag, true);
		this.ApplyForce(e, Direction.Down, 1, 1, true, false);
		Coord coord = e.pos + Coord.Down - direction;
		Direction direction2 = this.LadderAt(coord);
		Coord coord2 = e.pos + Coord.Down;
		Entity entity2 = this.EntAt(coord2, true, false);
		bool flag2 = entity2 != null && entity2.moving && entity2.movement.translation && entity2.movement.direction == Direction.Down;
		if ((direction2 != direction && entity2 == null) || flag2 || this.BBQAt(coord2))
		{
			this.RestoreEntities();
			this.AddTranslation(e, Direction.Up, 0, 1, Movement.MType.ClimbUp_Loop, !flag, true);
			this.ApplyForce(e, Direction.Up, 0, 1, true, false);
			e.dat = ((int)direction.Inverse()).ToStringFast();
		}
		else if (e.Collides(true, false) || (entity != null && entity.movement != null))
		{
			this.RestoreEntities();
			coord = e.pos - direction;
			Direction direction3 = this.LadderAt(e.pos - direction);
			if (direction3 == direction)
			{
				coord = e.pos + Coord.Down - direction;
				Direction direction4 = this.LadderAt(coord);
				if (direction4 == direction)
				{
					this.AddTranslation(e, Direction.Up, 0, 1, Movement.MType.ClimbUp_Loop, !flag, true);
					this.ApplyForce(e, Direction.Up, 0, 1, true, false);
					e.dat = ((int)direction.Inverse()).ToStringFast();
				}
			}
			else
			{
				this.BakEntities();
				this.ApplyForce(e, direction.Inverse(), 1, 1, true, false);
				this.AddTranslation(e, direction.Inverse(), 0, 1, Movement.MType.ClimbUp_End2, !flag, true);
				if (e.Collides(true, false))
				{
					this.RestoreEntities();
					e.dat = string.Empty;
				}
				else
				{
					this.DiscardLastBackup();
					e.dat = string.Empty;
				}
			}
		}
		else if (!this.Floating(e.TargetPos()))
		{
			this.DiscardLastBackup();
			e.dat = string.Empty;
		}
		else
		{
			this.DiscardLastBackup();
		}
	}

	// Token: 0x06000A48 RID: 2632 RVA: 0x0002F844 File Offset: 0x0002DC44
	private void AutomaticPlayerTick(bool wasclimbing)
	{
		if (this.player.movement != null)
		{
			return;
		}
		this.TryReattachFork();
		if (this.player.Turning())
		{
			this.AutomaticTurn(this.player, 2);
		}
		else if (!this.HasFooting(this.player) || wasclimbing)
		{
			if (this.player.dat.Length > 0)
			{
				int num = this.player.dat.IntParseFast();
				if (num >= 0)
				{
					this.AutomaticClimbUp(this.player);
				}
				else
				{
					this.AutomaticClimbDown(this.player);
				}
			}
		}
		else if (this.BBQAt(this.player.pos + Direction.Down))
		{
			Direction direction = this.lastdir;
			if (direction.Horizontal())
			{
				if (!this.ProcessInput(direction.Inverse()))
				{
					this.ProcessInput(direction);
				}
			}
			else if (!this.ProcessInput(this.player.direction))
			{
				this.ProcessInput(this.player.direction.Inverse());
			}
			this.doouch = true;
			this.smokes.Add(this.player.pos);
		}
	}

	// Token: 0x06000A49 RID: 2633 RVA: 0x0002F990 File Offset: 0x0002DD90
	public bool CanFall(Entity e, bool recurse = true)
	{
		if (e.pos.z < -10 || e.movement != null)
		{
			return false;
		}
		if (e.type == EntType.sausage)
		{
			if (this.Floating(e) && (!recurse || !this.Laden(e) || !this.LadenTarget(e).Extended()))
			{
				return true;
			}
		}
		else if (e.type == EntType.player)
		{
			if (e.dat.Length > 0)
			{
				return false;
			}
			if (this.Floating(e))
			{
				if (!this.Laden(e))
				{
					return true;
				}
				if (this.CanFall(this.LadenTarget(e), false))
				{
					return true;
				}
			}
		}
		else if (e.type == EntType.fork)
		{
			if (this.Floating(e))
			{
				return true;
			}
		}
		else if (e.type == EntType.island)
		{
			return (!this.overworld || this.pushestotry > 0) && !(this.pushtargetlevel == e.dat);
		}
		return false;
	}

	// Token: 0x06000A4A RID: 2634 RVA: 0x0002FAB0 File Offset: 0x0002DEB0
	public bool CanFall_Liberal(Entity e, bool recurse = true)
	{
		if (e.movement != null || e.Bottom() < -10)
		{
			return false;
		}
		if (e.type == EntType.island)
		{
			return !this.overworld && this.pushestotry == 0 && !(this.pushtargetlevel == e.dat);
		}
		if (e.type != EntType.sausage)
		{
			if (e.type == EntType.player && e.dat.Length > 0)
			{
				return false;
			}
		}
		return !recurse || e.stuckto < 0 || this.CanFall_Liberal(e.LadenTarget(), false);
	}

	// Token: 0x06000A4B RID: 2635 RVA: 0x0002FB66 File Offset: 0x0002DF66
	public bool Laden(Entity e)
	{
		return e.stuckto >= 0;
	}

	// Token: 0x06000A4C RID: 2636 RVA: 0x0002FB74 File Offset: 0x0002DF74
	public Entity LadenTarget(Entity e)
	{
		if (e.stuckto == -1)
		{
			return null;
		}
		return this.FromIDDynamic(e.stuckto);
	}

	// Token: 0x06000A4D RID: 2637 RVA: 0x0002FB90 File Offset: 0x0002DF90
	public bool IsEntAt(Coord pos, bool instant = false)
	{
		return this.EntAt(pos, instant, false) != null;
	}

	// Token: 0x06000A4E RID: 2638 RVA: 0x0002FBA4 File Offset: 0x0002DFA4
	public bool SolidEntAt(Coord pos, bool instant = false, bool ignoreladenforks = false)
	{
		Entity entity = this.EntAt(pos, instant, ignoreladenforks);
		return entity != null && entity.Solid();
	}

	// Token: 0x06000A4F RID: 2639 RVA: 0x0002FBCC File Offset: 0x0002DFCC
	public Entity EntAt(Coord pos, bool instant = false, bool ignoreladenforks = false)
	{
		HashSet<Entity> hashSet = this.BoxAt(pos);
		if (hashSet != null)
		{
			foreach (Entity entity in hashSet)
			{
				if (entity.At(pos, instant, false, false))
				{
					if (!ignoreladenforks || entity.type != EntType.fork || !entity.Laden())
					{
						return entity;
					}
				}
			}
		}
		return this.StaticEntAt(pos);
	}

	// Token: 0x06000A50 RID: 2640 RVA: 0x0002FC44 File Offset: 0x0002E044
	private List<Entity> EntsAt_Debug(Coord pos, bool instant = false)
	{
		if (!Application.isEditor)
		{
			Debug.LogError("calling entsat_debug from standalone game");
		}
		return this.entities.Where<Entity>((Entity e) => e.At(pos, instant, true, false)).ToList<Entity>();
	}

	// Token: 0x06000A51 RID: 2641 RVA: 0x0002FC98 File Offset: 0x0002E098
	public List<Entity> EntsAt_NoAlloc(Coord pos, bool instant = false, bool weak = false)
	{
		this._EntsAt_NoAlloc_Result.Clear();
		HashSet<Entity> hashSet = this.BoxAt(pos);
		if (hashSet != null)
		{
			foreach (Entity entity in hashSet)
			{
				if (entity.At(pos, instant, false, weak))
				{
					this._EntsAt_NoAlloc_Result.Add(entity);
				}
			}
		}
		return this._EntsAt_NoAlloc_Result;
	}

	// Token: 0x06000A52 RID: 2642 RVA: 0x0002FCFF File Offset: 0x0002E0FF
	public void EntsAt_Dealloc()
	{
		this._EntsAt_Level--;
	}

	// Token: 0x06000A53 RID: 2643 RVA: 0x0002FD10 File Offset: 0x0002E110
	public List<Entity> EntsAt_Alloc(Coord pos, bool instant = false, bool weak = false)
	{
		if (this._EntsAt_Ents.Count <= this._EntsAt_Level)
		{
			this._EntsAt_Ents.Add(new List<Entity>(2));
		}
		List<Entity> list = this._EntsAt_Ents[this._EntsAt_Level];
		list.Clear();
		this._EntsAt_Level++;
		HashSet<Entity> hashSet = this.BoxAt(pos);
		if (hashSet != null)
		{
			foreach (Entity entity in hashSet)
			{
				if (entity.At(pos, instant, false, weak))
				{
					list.Add(entity);
				}
			}
		}
		return list;
	}

	// Token: 0x06000A54 RID: 2644 RVA: 0x0002FDB0 File Offset: 0x0002E1B0
	public List<Entity> EntsAt_NoAlloc(Coord[] pos, bool instant = false)
	{
		this._EntsAt_NoAlloc_List.Clear();
		BoundingBox boundingBox = new BoundingBox(pos);
		List<HashSet<Entity>> list = this.BoxNeighbours(boundingBox);
		for (int i = 0; i < list.Count; i++)
		{
			HashSet<Entity> hashSet = list[i];
			foreach (Entity entity in hashSet)
			{
				if (entity.At(pos, instant, true))
				{
					this._EntsAt_NoAlloc_List.Add(entity);
				}
			}
		}
		return this._EntsAt_NoAlloc_List;
	}

	// Token: 0x06000A55 RID: 2645 RVA: 0x0002FE3C File Offset: 0x0002E23C
	public List<Entity> EntsAt(Coord[] pos, bool instant = false)
	{
		List<Entity> list = new List<Entity>(2);
		BoundingBox boundingBox = new BoundingBox(pos);
		List<HashSet<Entity>> list2 = this.BoxNeighbours(boundingBox);
		for (int i = 0; i < list2.Count; i++)
		{
			HashSet<Entity> hashSet = list2[i];
			foreach (Entity entity in hashSet)
			{
				if (entity.At(pos, instant, true))
				{
					list.Add(entity);
				}
			}
		}
		return list;
	}

	// Token: 0x06000A56 RID: 2646 RVA: 0x0002FEBC File Offset: 0x0002E2BC
	public List<Entity> EntsAt(BoundingBox bbox, Coord[] pos, bool instant = false)
	{
		List<Entity> list = new List<Entity>();
		List<HashSet<Entity>> list2 = this.BoxNeighbours(bbox);
		for (int i = 0; i < list2.Count; i++)
		{
			HashSet<Entity> hashSet = list2[i];
			foreach (Entity entity in hashSet)
			{
				if (entity.At(pos, instant, true))
				{
					list.Add(entity);
				}
			}
		}
		return list;
	}

	// Token: 0x06000A57 RID: 2647 RVA: 0x0002FF30 File Offset: 0x0002E330
	public bool HasAt(Coord pos, EntType type, bool instant = false)
	{
		return this.EntsAt_NoAlloc(pos, instant, false).Any<Entity>((Entity e) => e.type == type);
	}

	// Token: 0x06000A58 RID: 2648 RVA: 0x0002FF64 File Offset: 0x0002E364
	public void RemoveEntity(Entity ent)
	{
		this.RemoveBox(ent);
		this.entitiesspawned = true;
		this.entities.Remove(ent);
		if (ent.type.Dynamic())
		{
			this.dynamicentities.Remove(ent);
			this.dynamicentityindex.Remove(ent.id);
			if (ent.type == EntType.island)
			{
				this.islandindex.Remove(ent.dat);
			}
			if (this.player != null && ent.id == this.player.id)
			{
				this.player = null;
			}
			if (this.fork != null && ent.id == this.fork.id)
			{
				this.fork = null;
				if (this.player != null)
				{
					this.player.cookdata = 0;
					this.player.Occupancy();
				}
			}
			foreach (Entity entity in this.dynamicentities)
			{
				if (entity.Laden() && entity.LadenTarget() == null)
				{
					entity.stuckto = -1;
					this.forkfork = true;
				}
			}
		}
	}

	// Token: 0x06000A59 RID: 2649 RVA: 0x00030098 File Offset: 0x0002E498
	public bool ClearCol(Coord pos, bool deleteislands = false, bool updateislands = true)
	{
		bool flag = false;
		for (int i = -2; i < 10; i++)
		{
			Coord coord = new Coord(pos.x, pos.y, i);
			flag |= this.Clear(coord, deleteislands, false);
		}
		if (updateislands)
		{
			this.PrecalcAll();
		}
		return flag;
	}

	// Token: 0x06000A5A RID: 2650 RVA: 0x000300EC File Offset: 0x0002E4EC
	public bool Clear(Coord pos, bool deleteislands = false, bool updateislands = true)
	{
		if (!Application.isEditor)
		{
			Debug.LogError("Error shouldn't call clear from game");
		}
		int num = this.entities.RemoveAll((Entity ent) => ent.At(pos, false, true, false) && ent.type != EntType.island);
		int num2 = this.dynamicentities.RemoveAll((Entity ent) => ent.At(pos, false, true, false) && ent.type != EntType.island);
		if (pos.x >= this.smapmin.x && pos.y >= this.smapmin.y && pos.z >= this.smapmin.z && pos.x <= this.smapmax.x && pos.y <= this.smapmax.y && pos.z <= this.smapmax.z && this.staticmap != null)
		{
			this.staticmap[pos.x - this.smapmin.x, pos.y - this.smapmin.y, pos.z - this.smapmin.z] = null;
		}
		if (num2 > 0)
		{
			this.UpdateDynamicIDDict();
			foreach (Entity entity in this.dynamicentities)
			{
				if (entity.LadenTarget() == null)
				{
					entity.stuckto = -1;
				}
			}
			if (this.fork != null && !this.dynamicentityindex.ContainsKey(this.fork.id))
			{
				this.fork = null;
				this.player.cookdata = 0;
				this.player.Occupancy();
				this.regenfork = true;
			}
		}
		if (this.player != null && this.player.pos == pos)
		{
			this.player = null;
		}
		if (this.metagame != null && updateislands)
		{
			this.PrecalcAll();
		}
		this.BuildBoxes();
		return num > 0;
	}

	// Token: 0x06000A5B RID: 2651 RVA: 0x0003032C File Offset: 0x0002E72C
	public void PrecalcAll()
	{
		if (this.metagame != null)
		{
			this.metagame.RegenIslands();
		}
		foreach (Entity entity in this.entities)
		{
			entity.PrecalcAll();
		}
		this.BuildBoxes();
	}

	// Token: 0x06000A5C RID: 2652 RVA: 0x00030380 File Offset: 0x0002E780
	public string Save(bool dynamiconly, bool normalize = false)
	{
		if (normalize)
		{
			this.NormalizeState(dynamiconly);
		}
		GameState._sb.Remove(0, GameState._sb.Length);
		if (!this.overworld)
		{
			GameState._sb.Append("I" + this.pushtargetlevel + "|");
		}
		if (dynamiconly)
		{
			foreach (Entity entity in this.dynamicentities)
			{
				entity.Save(GameState._sb);
			}
		}
		else
		{
			foreach (Entity entity2 in this.entities)
			{
				entity2.Save(GameState._sb);
			}
		}
		GameState._sb.Append('*');
		foreach (string text in this.levelcompleted)
		{
			GameState._sb.Append(text);
			GameState._sb.Append(',');
		}
		GameState._sb.Append('*');
		foreach (string text2 in this.worldsausagesissued)
		{
			GameState._sb.Append(text2);
			GameState._sb.Append(',');
		}
		GameState._sb.Append('*');
		GameState._sb.Append(this.tileset.ToStringFast());
		GameState._sb.Append('*');
		GameState._sb.Append(this.displayname);
		GameState._sb.Append('*');
		GameState._sb.Append(this.sausagescooked.ToStringFast());
		GameState._sb.Append('*');
		GameState._sb.Append(this.musicseed);
		return GameState._sb.ToString();
	}

	// Token: 0x06000A5D RID: 2653 RVA: 0x000305B0 File Offset: 0x0002E9B0
	private void LoadDat(string s_big, MetaGameState _metagame, bool precalc = true)
	{
		string[] array = s_big.Split(new char[] { '*' });
		string[] array2 = array[0].Split(new char[] { '|' }, StringSplitOptions.RemoveEmptyEntries);
		this.metagame = _metagame;
		if (array2.Length == 0)
		{
			this.entities = new List<Entity>();
			this.dynamicentities = new List<Entity>();
			this.dynamicentityindex = new IntDictionary<Entity>();
			return;
		}
		if (array2[0][0] == 'F')
		{
			string[] array3 = new string[array2.Length - 1];
			Array.Copy(array2, 1, array3, 0, array2.Length - 1);
			array2 = array3;
		}
		else if (array2[0][0] == 'I')
		{
			this.overworld = false;
			this.pushtargetlevel = array2[0].Substring(1);
			string[] array4 = new string[array2.Length - 1];
			Array.Copy(array2, 1, array4, 0, array2.Length - 1);
			array2 = array4;
		}
		else
		{
			this.overworld = true;
		}
		this.entities = new List<Entity>(array2.Length);
		this.dynamicentities = new List<Entity>(array2.Length);
		this.dynamicentityindex = new IntDictionary<Entity>(array2.Length);
		foreach (string text in array2)
		{
			Entity entity = Entity.Load(text, this, precalc);
			this.entities.Add(entity);
			if (entity.type.Dynamic())
			{
				entity._movement = new Movement();
				this.dynamicentities.Add(entity);
				while (this.dynamicentityindex.ContainsKey(entity.id))
				{
					entity.id++;
				}
				this.dynamicentityindex[entity.id] = entity;
				if (entity.type == EntType.island)
				{
					this.islandindex[entity.dat] = entity;
				}
			}
		}
		this.player = this.dynamicentities.FirstOrDefault<Entity>((Entity e) => e.type == EntType.player);
		this.fork = this.dynamicentities.FirstOrDefault<Entity>((Entity e) => e.type == EntType.fork);
		if (precalc)
		{
			this.UpdateSMap();
		}
		int num;
		if (this.entities.Count > 0)
		{
			num = this.entities.Max<Entity>((Entity e) => e.id) + 1;
		}
		else
		{
			num = 0;
		}
		this.idcounter = num;
		if (array.Length > 2 && (array[1].Length > 0 || array[2].Length > 0))
		{
			this.levelcompleted = new List<string>(array[1].Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries));
			this.worldsausagesissued = new List<string>(array[2].Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries));
		}
		else
		{
			this.levelcompleted.Clear();
			this.worldsausagesissued.Clear();
		}
		if (array.Length > 3 && array[3].Length > 0)
		{
			this.tileset = array[3].IntParseFast();
		}
		else
		{
			this.tileset = 0;
		}
		if (array.Length > 4 && array[4].Length > 0)
		{
			this.displayname = array[4];
		}
		else
		{
			this.displayname = string.Empty;
		}
		if (array.Length > 5)
		{
			this.sausagescooked = array[5].IntParseFast();
		}
		else
		{
			this.sausagescooked = 0;
		}
		if (array.Length > 6)
		{
			this.musicseed = array[6].IntParseFast();
		}
		else
		{
			this.musicseed = 0;
		}
		this.exitPos = Coord.Invalid;
		this.exitDir = Direction.None;
		this.exitAttachment = null;
		if (precalc && this.player != null && this.metagame != null)
		{
			this.PassiveForceSweep(true);
		}
		if (precalc)
		{
			this.BuildBoxes();
		}
	}

	// Token: 0x06000A5E RID: 2654 RVA: 0x0003099D File Offset: 0x0002ED9D
	public static GameState Load(string s, MetaGameState _metagame, bool precalc = true)
	{
		if (s.Length == 0)
		{
			return GameState.Blank(_metagame);
		}
		if (s[0] == 'V')
		{
			return null;
		}
		return new GameState(s, _metagame, precalc);
	}

	// Token: 0x06000A5F RID: 2655 RVA: 0x000309CC File Offset: 0x0002EDCC
	public IntDictionary<List<Coord>> CoastData()
	{
		IntDictionary<List<Coord>> intDictionary = new IntDictionary<List<Coord>>();
		IEnumerable<Entity> coastents = this.entities.Where<Entity>((Entity e) => e.type.Static() && !e.Decoration());
		foreach (Entity entity in coastents)
		{
			Coord[] array = new Coord[]
			{
				entity.pos + Direction.North,
				entity.pos + Direction.South,
				entity.pos + Direction.East,
				entity.pos + Direction.West
			};
			if (array.Any<Coord>((Coord nb) => !coastents.Any<Entity>((Entity ce) => ce.pos == nb)))
			{
				if (!intDictionary.ContainsKey(entity.pos.z))
				{
					intDictionary.Add(entity.pos.z, new List<Coord>());
				}
				intDictionary[entity.pos.z].Add(entity.pos);
			}
		}
		return intDictionary;
	}

	// Token: 0x06000A60 RID: 2656 RVA: 0x00030B24 File Offset: 0x0002EF24
	public IntDictionary<List<Coord>> SplashData()
	{
		IntDictionary<List<Coord>> intDictionary = new IntDictionary<List<Coord>>();
		IEnumerable<Entity> enumerable = this.entities.Where<Entity>((Entity e) => e.type.Static() && !e.Decoration());
		using (IEnumerator<Entity> enumerator = enumerable.GetEnumerator())
		{
			while (enumerator.MoveNext())
			{
				Entity e = enumerator.Current;
				if (!enumerable.Any<Entity>((Entity ce) => ce.pos == e.pos + Direction.Up))
				{
					if (!intDictionary.ContainsKey(e.pos.z))
					{
						intDictionary.Add(e.pos.z, new List<Coord>());
					}
					intDictionary[e.pos.z].Add(e.pos);
				}
			}
		}
		return intDictionary;
	}

	// Token: 0x06000A61 RID: 2657 RVA: 0x00030C20 File Offset: 0x0002F020
	public void SetIDCounterTo(int newval)
	{
		this.idcounter = newval;
	}

	// Token: 0x06000A62 RID: 2658 RVA: 0x00030C29 File Offset: 0x0002F029
	public void IncreaseIDCounterTo(int newval)
	{
		this.idcounter = Math.Max(this.idcounter, newval);
	}

	// Token: 0x06000A63 RID: 2659 RVA: 0x00030C3D File Offset: 0x0002F03D
	public int GetNewID()
	{
		this.idcounter++;
		return this.idcounter - 1;
	}

	// Token: 0x06000A64 RID: 2660 RVA: 0x00030C55 File Offset: 0x0002F055
	public int NextID()
	{
		return this.idcounter;
	}

	// Token: 0x06000A65 RID: 2661 RVA: 0x00030C60 File Offset: 0x0002F060
	public void StripDecorations(Coord vanillatile)
	{
		this.tileset = 0;
		foreach (Entity entity in this.entities)
		{
			if (entity.type == EntType.ground)
			{
				entity.tileset = vanillatile.y;
				entity.tilenum = vanillatile.z;
			}
			else if (entity.type == EntType.bbq)
			{
				entity.tileset = vanillatile.y;
				entity.tilenum = 0;
			}
		}
		this.entities.RemoveAll((Entity e) => e.Decoration() || e.type == EntType.spectralsausage);
		this.dynamicentities.RemoveAll((Entity e) => e.Decoration() || e.type == EntType.spectralsausage);
		this.UpdateSMap();
	}

	// Token: 0x040005BF RID: 1471
	public MetaGameState metagame;

	// Token: 0x040005C0 RID: 1472
	public List<Entity> entities;

	// Token: 0x040005C1 RID: 1473
	public List<Entity> dynamicentities;

	// Token: 0x040005C2 RID: 1474
	public List<Movement> movements = new List<Movement>(250);

	// Token: 0x040005C3 RID: 1475
	public List<Coord> sparks = new List<Coord>(50);

	// Token: 0x040005C4 RID: 1476
	public List<Coord> landingPuffs = new List<Coord>(10);

	// Token: 0x040005C5 RID: 1477
	public List<Coord> smokes = new List<Coord>(50);

	// Token: 0x040005C6 RID: 1478
	public List<Coord> splashes = new List<Coord>(50);

	// Token: 0x040005C7 RID: 1479
	public List<Coord> sausageexplosions = new List<Coord>(10);

	// Token: 0x040005C8 RID: 1480
	public List<Direction> sausagedirs = new List<Direction>(10);

	// Token: 0x040005C9 RID: 1481
	public List<Coord> worldsausagespawns = new List<Coord>(10);

	// Token: 0x040005CA RID: 1482
	public List<Direction> worldsausagespawndirs = new List<Direction>(10);

	// Token: 0x040005CB RID: 1483
	public List<Coord> petals = new List<Coord>(100);

	// Token: 0x040005CC RID: 1484
	public List<int> petaltypes = new List<int>(100);

	// Token: 0x040005CD RID: 1485
	public List<string> levelcompleted = new List<string>(200);

	// Token: 0x040005CE RID: 1486
	public List<string> worldsausagesissued = new List<string>(200);

	// Token: 0x040005CF RID: 1487
	public int anyrolls;

	// Token: 0x040005D0 RID: 1488
	public int anydrags;

	// Token: 0x040005D1 RID: 1489
	public int anyislandmoves;

	// Token: 0x040005D2 RID: 1490
	public int anylandings;

	// Token: 0x040005D3 RID: 1491
	public int anyforklandings;

	// Token: 0x040005D4 RID: 1492
	public int playerwalk;

	// Token: 0x040005D5 RID: 1493
	public int playerbackpedal;

	// Token: 0x040005D6 RID: 1494
	public int playerturn;

	// Token: 0x040005D7 RID: 1495
	public const int ashStepMax = 4;

	// Token: 0x040005D8 RID: 1496
	public int oldAshStepCount;

	// Token: 0x040005D9 RID: 1497
	public int ashStepCount;

	// Token: 0x040005DA RID: 1498
	public bool playerclimbup;

	// Token: 0x040005DB RID: 1499
	public int playerclimbdownland;

	// Token: 0x040005DC RID: 1500
	public bool regenfork;

	// Token: 0x040005DD RID: 1501
	public bool addfork;

	// Token: 0x040005DE RID: 1502
	public bool forkfork;

	// Token: 0x040005DF RID: 1503
	public bool doouch;

	// Token: 0x040005E0 RID: 1504
	public bool recalccoastsplashes;

	// Token: 0x040005E1 RID: 1505
	public static Coord forkDepot = new Coord(-1000, -1000, -1000);

	// Token: 0x040005E2 RID: 1506
	public bool startedrising;

	// Token: 0x040005E3 RID: 1507
	public bool startedfalling;

	// Token: 0x040005E4 RID: 1508
	public int donelowering;

	// Token: 0x040005E5 RID: 1509
	public bool sausagelost;

	// Token: 0x040005E6 RID: 1510
	public bool forksfx;

	// Token: 0x040005E7 RID: 1511
	public bool unforksfx;

	// Token: 0x040005E8 RID: 1512
	public bool attachforksfx;

	// Token: 0x040005E9 RID: 1513
	public bool detatchforksfx;

	// Token: 0x040005EA RID: 1514
	public bool drownsfx;

	// Token: 0x040005EB RID: 1515
	public Entity[,,] staticmap;

	// Token: 0x040005EC RID: 1516
	public Coord smapmin = new Coord(0, 0, 0);

	// Token: 0x040005ED RID: 1517
	public Coord smapmax = new Coord(0, 0, 0);

	// Token: 0x040005EE RID: 1518
	public bool overworld = true;

	// Token: 0x040005EF RID: 1519
	public int tileset;

	// Token: 0x040005F0 RID: 1520
	public string displayname = string.Empty;

	// Token: 0x040005F1 RID: 1521
	public int sausagescooked;

	// Token: 0x040005F2 RID: 1522
	public int musicseed;

	// Token: 0x040005F3 RID: 1523
	public Coord exitPos;

	// Token: 0x040005F4 RID: 1524
	public Direction exitDir;

	// Token: 0x040005F5 RID: 1525
	public bool exitUp;

	// Token: 0x040005F6 RID: 1526
	public Entity exitAttachment;

	// Token: 0x040005F7 RID: 1527
	public const int maxpushes = 22;

	// Token: 0x040005F8 RID: 1528
	public int pushestotry;

	// Token: 0x040005F9 RID: 1529
	public string pushtargetlevel = string.Empty;

	// Token: 0x040005FA RID: 1530
	private static string lastpushed = string.Empty;

	// Token: 0x040005FB RID: 1531
	public static Coord shouldredrawcoffins = Coord.Invalid;

	// Token: 0x040005FC RID: 1532
	public static bool shoulddespawnstarts = false;

	// Token: 0x040005FD RID: 1533
	public bool won;

	// Token: 0x040005FE RID: 1534
	public bool returning;

	// Token: 0x040005FF RID: 1535
	public bool haveevercookedall;

	// Token: 0x04000600 RID: 1536
	public string lostreason = string.Empty;

	// Token: 0x04000601 RID: 1537
	[HideInInspector]
	public IntDictionary<Entity> dynamicentityindex = new IntDictionary<Entity>();

	// Token: 0x04000602 RID: 1538
	[HideInInspector]
	public StringDictionary<Entity> islandindex = new StringDictionary<Entity>();

	// Token: 0x04000603 RID: 1539
	public bool entitiesspawned = true;

	// Token: 0x04000604 RID: 1540
	public static EntitySkeleton[] bakentsstock;

	// Token: 0x04000605 RID: 1541
	public static int bakentindex = -1;

	// Token: 0x04000606 RID: 1542
	public const int backupsize = 10000;

	// Token: 0x04000607 RID: 1543
	private List<GameState.BakStruct> backup = new List<GameState.BakStruct>();

	// Token: 0x04000608 RID: 1544
	private static int _baklayer = -1;

	// Token: 0x04000609 RID: 1545
	private static List<EntitySkeleton[]> _bakskels = new List<EntitySkeleton[]>();

	// Token: 0x0400060A RID: 1546
	private static List<List<Movement>> _bakmovements = new List<List<Movement>>();

	// Token: 0x0400060B RID: 1547
	public IntDictionary<Direction> moveattempts = new IntDictionary<Direction>();

	// Token: 0x0400060C RID: 1548
	private List<Entity> totrycook = new List<Entity>();

	// Token: 0x0400060D RID: 1549
	private Direction lastdir;

	// Token: 0x0400060E RID: 1550
	private static int lastftype = 1;

	// Token: 0x0400060F RID: 1551
	private const int TORSIONPLACEHOLDER = -666;

	// Token: 0x04000610 RID: 1552
	private int _boxneighbours_af_index = -1;

	// Token: 0x04000611 RID: 1553
	private static List<List<HashSet<Entity>>> _boxneighbours_af = new List<List<HashSet<Entity>>>(32);

	// Token: 0x04000612 RID: 1554
	private static List<Entity> _underents = new List<Entity>();

	// Token: 0x04000613 RID: 1555
	private List<Entity> fallers = new List<Entity>();

	// Token: 0x04000614 RID: 1556
	private bool wasraising;

	// Token: 0x04000615 RID: 1557
	public bool waslowering;

	// Token: 0x04000616 RID: 1558
	private static List<Entity> _footprintents = new List<Entity>();

	// Token: 0x04000617 RID: 1559
	private static List<HashSet<Entity>> _footprintents_neighbours = new List<HashSet<Entity>>();

	// Token: 0x04000618 RID: 1560
	private static List<Entity> _footprint_ents = new List<Entity>(5);

	// Token: 0x04000619 RID: 1561
	public IntDictionary<BoundingBox> bboxes;

	// Token: 0x0400061A RID: 1562
	public int idcounter;

	// Token: 0x0400061B RID: 1563
	public int curtowerlevel;

	// Token: 0x0400061C RID: 1564
	private static List<IntDictionary<BoundingBox>> bboxes_tower = new List<IntDictionary<BoundingBox>>(32);

	// Token: 0x0400061D RID: 1565
	private static List<List<HashSet<Entity>>> boxneighbours_tower = new List<List<HashSet<Entity>>>(32);

	// Token: 0x0400061E RID: 1566
	private static List<List<Entity>> _applyforce_footprints_ents = new List<List<Entity>>(10);

	// Token: 0x0400061F RID: 1567
	public IntDictionary<HashSet<Entity>> boxes = new IntDictionary<HashSet<Entity>>();

	// Token: 0x04000620 RID: 1568
	public const int BOXSIZE = 10;

	// Token: 0x04000621 RID: 1569
	public const int MaxX = 10000;

	// Token: 0x04000622 RID: 1570
	private List<Entity> overwatermovements = new List<Entity>(100);

	// Token: 0x04000623 RID: 1571
	private static IntDictionary<KeyValuePair<int, float>> cacheddat = new IntDictionary<KeyValuePair<int, float>>();

	// Token: 0x04000624 RID: 1572
	private static Comparison<Entity> _dynamicSortFn = delegate(Entity ent1, Entity ent2)
	{
		KeyValuePair<int, float> keyValuePair = GameState.cacheddat[ent1.id];
		KeyValuePair<int, float> keyValuePair2 = GameState.cacheddat[ent2.id];
		int num = (-keyValuePair.Key).CompareTo(-keyValuePair2.Key);
		if (num == 0)
		{
			num = (-keyValuePair.Value).CompareTo(-keyValuePair2.Value);
		}
		return num;
	};

	// Token: 0x04000625 RID: 1573
	private bool nopassivesweeps;

	// Token: 0x04000626 RID: 1574
	public Entity player;

	// Token: 0x04000627 RID: 1575
	public Entity fork;

	// Token: 0x04000628 RID: 1576
	private List<Entity> _EntsAt_NoAlloc_Result = new List<Entity>(2);

	// Token: 0x04000629 RID: 1577
	private List<List<Entity>> _EntsAt_Ents = new List<List<Entity>>();

	// Token: 0x0400062A RID: 1578
	private int _EntsAt_Level;

	// Token: 0x0400062B RID: 1579
	private List<Entity> _EntsAt_NoAlloc_List = new List<Entity>(4);

	// Token: 0x0400062C RID: 1580
	private static StringBuilder _sb = new StringBuilder();

	// Token: 0x020000A2 RID: 162
	public struct BakStruct
	{
		// Token: 0x06000A8F RID: 2703 RVA: 0x0003130C File Offset: 0x0002F70C
		public BakStruct(EntitySkeleton[] ents, List<Movement> movements, string[] levelcompleted, string[] worldsausagesissued, string pushtargetlevel, bool overworld, bool won, bool returning, bool haveevercookedall, int tileset, GameState gs_effects, string lostreason)
		{
			this.ents = ents;
			this.movements = movements;
			this.levelcompleted = levelcompleted;
			this.worldsausagesissued = worldsausagesissued;
			this.overworld = overworld;
			this.won = won;
			this.returning = returning;
			this.haveevercookedall = haveevercookedall;
			this.pushtargetlevel = pushtargetlevel;
			this.tileset = tileset;
			this.anyrolls = gs_effects.anyrolls;
			this.anydrags = gs_effects.anydrags;
			this.anyislandmoves = gs_effects.anyislandmoves;
			this.anylandings = gs_effects.anylandings;
			this.anyforklandings = gs_effects.anyforklandings;
			this.playerwalk = gs_effects.playerwalk;
			this.playerbackpedal = gs_effects.playerbackpedal;
			this.playerturn = gs_effects.playerturn;
			this.oldAshStepCount = gs_effects.oldAshStepCount;
			this.ashStepCount = gs_effects.ashStepCount;
			this.playerclimbup = gs_effects.playerclimbup;
			this.playerclimbdownland = gs_effects.playerclimbdownland;
			this.regenfork = gs_effects.regenfork;
			this.addfork = gs_effects.addfork;
			this.forkfork = gs_effects.forkfork;
			this.doouch = gs_effects.doouch;
			this.recalccoastsplashes = gs_effects.recalccoastsplashes;
			this.startedrising = gs_effects.startedrising;
			this.startedfalling = gs_effects.startedfalling;
			this.donelowering = gs_effects.donelowering;
			this.sausagelost = gs_effects.sausagelost;
			this.forksfx = gs_effects.forksfx;
			this.unforksfx = gs_effects.unforksfx;
			this.attachforksfx = gs_effects.attachforksfx;
			this.detatchforksfx = gs_effects.detatchforksfx;
			this.drownsfx = gs_effects.drownsfx;
			this.displayname = gs_effects.displayname;
			this.musicseed = gs_effects.musicseed;
			this.sausagescooked = gs_effects.sausagescooked;
			this.exitPos = gs_effects.exitPos;
			this.exitDir = gs_effects.exitDir;
			this.exitUp = gs_effects.exitUp;
			this.exitTargetPos = ((gs_effects.exitAttachment != null) ? gs_effects.exitAttachment.pos : Coord.Invalid);
			this.lostreason = lostreason;
		}

		// Token: 0x06000A90 RID: 2704 RVA: 0x00031538 File Offset: 0x0002F938
		public static bool operator ==(GameState.BakStruct a, GameState.BakStruct b)
		{
			int num = a.ents.Count<EntitySkeleton>();
			int num2 = b.ents.Count<EntitySkeleton>();
			if (num != num2)
			{
				return false;
			}
			for (int i = 0; i < num; i++)
			{
				EntitySkeleton entitySkeleton = a.ents[i];
				EntitySkeleton entitySkeleton2 = b.ents[i];
				if (entitySkeleton.pos != entitySkeleton2.pos || entitySkeleton.direction != entitySkeleton2.direction || entitySkeleton.cookdata != entitySkeleton2.cookdata)
				{
					return false;
				}
			}
			return true;
		}

		// Token: 0x06000A91 RID: 2705 RVA: 0x000315CD File Offset: 0x0002F9CD
		public static bool operator !=(GameState.BakStruct a, GameState.BakStruct b)
		{
			return !(a == b);
		}

		// Token: 0x06000A92 RID: 2706 RVA: 0x000315D9 File Offset: 0x0002F9D9
		public override bool Equals(object o)
		{
			return o != null && o.GetType() == typeof(GameState.BakStruct) && (GameState.BakStruct)o == this;
		}

		// Token: 0x06000A93 RID: 2707 RVA: 0x00031609 File Offset: 0x0002FA09
		public override int GetHashCode()
		{
			return this.ents.GetHashCode();
		}

		// Token: 0x04000652 RID: 1618
		public readonly int anyrolls;

		// Token: 0x04000653 RID: 1619
		public readonly int anydrags;

		// Token: 0x04000654 RID: 1620
		public readonly int anyislandmoves;

		// Token: 0x04000655 RID: 1621
		public readonly int anylandings;

		// Token: 0x04000656 RID: 1622
		public readonly int anyforklandings;

		// Token: 0x04000657 RID: 1623
		public readonly int playerwalk;

		// Token: 0x04000658 RID: 1624
		public readonly int playerbackpedal;

		// Token: 0x04000659 RID: 1625
		public readonly int playerturn;

		// Token: 0x0400065A RID: 1626
		public readonly int oldAshStepCount;

		// Token: 0x0400065B RID: 1627
		public readonly int ashStepCount;

		// Token: 0x0400065C RID: 1628
		public readonly bool playerclimbup;

		// Token: 0x0400065D RID: 1629
		public readonly int playerclimbdownland;

		// Token: 0x0400065E RID: 1630
		public readonly bool regenfork;

		// Token: 0x0400065F RID: 1631
		public readonly bool addfork;

		// Token: 0x04000660 RID: 1632
		public readonly bool forkfork;

		// Token: 0x04000661 RID: 1633
		public readonly bool doouch;

		// Token: 0x04000662 RID: 1634
		public readonly bool recalccoastsplashes;

		// Token: 0x04000663 RID: 1635
		public readonly bool startedrising;

		// Token: 0x04000664 RID: 1636
		public readonly bool startedfalling;

		// Token: 0x04000665 RID: 1637
		public readonly int donelowering;

		// Token: 0x04000666 RID: 1638
		public readonly bool sausagelost;

		// Token: 0x04000667 RID: 1639
		public readonly bool forksfx;

		// Token: 0x04000668 RID: 1640
		public readonly bool unforksfx;

		// Token: 0x04000669 RID: 1641
		public readonly bool attachforksfx;

		// Token: 0x0400066A RID: 1642
		public readonly bool detatchforksfx;

		// Token: 0x0400066B RID: 1643
		public readonly bool drownsfx;

		// Token: 0x0400066C RID: 1644
		public readonly EntitySkeleton[] ents;

		// Token: 0x0400066D RID: 1645
		public readonly List<Movement> movements;

		// Token: 0x0400066E RID: 1646
		public readonly string[] levelcompleted;

		// Token: 0x0400066F RID: 1647
		public readonly string[] worldsausagesissued;

		// Token: 0x04000670 RID: 1648
		public readonly bool overworld;

		// Token: 0x04000671 RID: 1649
		public readonly bool won;

		// Token: 0x04000672 RID: 1650
		public readonly bool returning;

		// Token: 0x04000673 RID: 1651
		public readonly bool haveevercookedall;

		// Token: 0x04000674 RID: 1652
		public readonly string pushtargetlevel;

		// Token: 0x04000675 RID: 1653
		public readonly int tileset;

		// Token: 0x04000676 RID: 1654
		public readonly string displayname;

		// Token: 0x04000677 RID: 1655
		public readonly int musicseed;

		// Token: 0x04000678 RID: 1656
		public readonly int sausagescooked;

		// Token: 0x04000679 RID: 1657
		public readonly Coord exitPos;

		// Token: 0x0400067A RID: 1658
		public readonly Direction exitDir;

		// Token: 0x0400067B RID: 1659
		public readonly bool exitUp;

		// Token: 0x0400067C RID: 1660
		public readonly Coord exitTargetPos;

		// Token: 0x0400067D RID: 1661
		public readonly string lostreason;
	}

	// Token: 0x020000A3 RID: 163
	private struct PassiveForce
	{
		// Token: 0x06000A94 RID: 2708 RVA: 0x00031616 File Offset: 0x0002FA16
		public PassiveForce(Direction _direction, int _speed, int _torsion)
		{
			this.direction = _direction;
			this.speed = _speed;
			this.torsion = _torsion;
		}

		// Token: 0x06000A95 RID: 2709 RVA: 0x00031630 File Offset: 0x0002FA30
		public override string ToString()
		{
			return string.Concat(new object[] { "PassiveForce:\n\tdirection : ", this.direction, "\n\tspeed : ", this.speed, "\n\ttorsion : ", this.torsion });
		}

		// Token: 0x0400067E RID: 1662
		public readonly Direction direction;

		// Token: 0x0400067F RID: 1663
		public readonly int speed;

		// Token: 0x04000680 RID: 1664
		public readonly int torsion;
	}
}
