using System.Drawing;
using CounterStrikeSharp.API;
using CounterStrikeSharp.API.Core;
using CounterStrikeSharp.API.Modules.Commands;
using CounterStrikeSharp.API.Modules.Utils;

namespace AdminESP;

public class AdminESP : BasePlugin
{
    public override string ModuleName        => "AdminESP";
    public override string ModuleVersion     => "1.0.0";
    public override string ModuleAuthor      => "sq1z";
    public override string ModuleDescription => "Admin ESP for AdminSystem";

    private readonly Dictionary<int, (CBaseModelEntity relay, CBaseModelEntity glow)> _glowProps = new();
    private readonly bool[] _espEnabled = new bool[65];

    public override void Load(bool hotReload)
    {
        RegisterListener<Listeners.CheckTransmit>(OnCheckTransmit);
        RegisterEventHandler<EventPlayerSpawn>(OnPlayerSpawn, HookMode.Post);
        RegisterEventHandler<EventPlayerDeath>(OnPlayerDeath);
        RegisterEventHandler<EventRoundStart>(OnRoundStart);
        RegisterListener<Listeners.OnClientDisconnect>(OnClientDisconnect);

        AddCommand("css_adminwh", "Toggle admin ESP for self", OnAdminWHCommand);
        AddCommand("css_adminwh_toggle", "Toggle admin ESP by slot", OnAdminWHToggleCommand);
    }

    public override void Unload(bool hotReload)
    {
        foreach (var slot in _glowProps.Keys.ToList())
            RemoveGlow(slot);
    }

    private void OnAdminWHCommand(CCSPlayerController? caller, CommandInfo info)
    {
        if (caller == null || !caller.IsValid) return;
        Toggle(caller);
    }

    private void OnAdminWHToggleCommand(CCSPlayerController? caller, CommandInfo info)
    {
        if (info.ArgCount < 2) return;
        if (!int.TryParse(info.GetArg(1), out int slot)) return;

        var player = Utilities.GetPlayerFromSlot(slot);
        if (player == null || !player.IsValid) return;

        Toggle(player);
    }

    private void Toggle(CCSPlayerController player)
    {
        _espEnabled[player.Slot] = !_espEnabled[player.Slot];

        if (_espEnabled[player.Slot])
        {
            foreach (var p in Utilities.GetPlayers())
            {
                if (p == null || !p.IsValid || !p.PawnIsAlive || p.Slot == player.Slot) continue;
                SetGlow(p);
            }
            player.PrintToChat(" \x04[AS]\x01 Admin ESP \x04ON");
        }
        else
        {
            foreach (var slot in _glowProps.Keys.ToList())
                RemoveGlow(slot);
            player.PrintToChat(" \x04[AS]\x01 Admin ESP \x02OFF");
        }
    }

    private void OnCheckTransmit(CCheckTransmitInfoList infoList)
    {
        foreach ((CCheckTransmitInfo info, CCSPlayerController? viewer) in infoList)
        {
            if (viewer == null || !viewer.IsValid) continue;
            if (_espEnabled[viewer.Slot]) continue;

            foreach (var (_, props) in _glowProps)
            {
                if (props.relay != null && props.relay.IsValid)
                    info.TransmitEntities.Remove((int)props.relay.Index);
                if (props.glow != null && props.glow.IsValid)
                    info.TransmitEntities.Remove((int)props.glow.Index);
            }
        }
    }

    private HookResult OnPlayerSpawn(EventPlayerSpawn @event, GameEventInfo info)
    {
        var player = @event.Userid;
        if (player == null || !player.IsValid) return HookResult.Continue;

        Server.NextFrame(() =>
        {
            if (player.PawnIsAlive)
                SetGlow(player);
        });

        return HookResult.Continue;
    }

    private HookResult OnPlayerDeath(EventPlayerDeath @event, GameEventInfo info)
    {
        var player = @event.Userid;
        if (player == null || !player.IsValid) return HookResult.Continue;
        RemoveGlow(player.Slot);
        return HookResult.Continue;
    }

    private HookResult OnRoundStart(EventRoundStart @event, GameEventInfo info)
    {
        foreach (var slot in _glowProps.Keys.ToList())
            RemoveGlow(slot);
        return HookResult.Continue;
    }

    private void OnClientDisconnect(int slot)
    {
        RemoveGlow(slot);
        _espEnabled[slot] = false;
    }

    private void SetGlow(CCSPlayerController player)
    {
        var pawn = player.PlayerPawn.Value;
        if (pawn == null || !pawn.IsValid) return;

        var sceneNode = pawn.CBodyComponent?.SceneNode;
        if (sceneNode == null) return;

        string modelName = sceneNode.GetSkeletonInstance().ModelState.ModelName;
        if (string.IsNullOrEmpty(modelName)) return;

        RemoveGlow(player.Slot);

        var relay = Utilities.CreateEntityByName<CBaseModelEntity>("prop_dynamic");
        var glow  = Utilities.CreateEntityByName<CBaseModelEntity>("prop_dynamic");
        if (relay == null || glow == null) return;

        relay.SetModel(modelName);
        relay.Spawnflags = 256u;
        relay.RenderMode = RenderMode_t.kRenderNone;
        relay.DispatchSpawn();

        glow.SetModel(modelName);
        glow.Spawnflags = 256u;
        glow.DispatchSpawn();

        glow.Glow.GlowColorOverride = player.TeamNum == 2 ? Color.Orange : Color.DeepSkyBlue;
        glow.Glow.GlowRange         = 5000;
        glow.Glow.GlowRangeMin      = 100;
        glow.Glow.GlowTeam          = -1;
        glow.Glow.GlowType          = 3;

        relay.AcceptInput("FollowEntity", pawn, relay, "!activator");
        glow.AcceptInput("FollowEntity", relay, glow, "!activator");

        _glowProps[player.Slot] = (relay, glow);
    }

    private void RemoveGlow(int slot)
    {
        if (!_glowProps.TryGetValue(slot, out var props)) return;
        if (props.relay != null && props.relay.IsValid) props.relay.AcceptInput("Kill");
        if (props.glow  != null && props.glow.IsValid)  props.glow.AcceptInput("Kill");
        _glowProps.Remove(slot);
    }
}
