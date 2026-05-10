#!/usr/bin/env perl
# rewrite-includes.pl — Sprint 10 cmangos/playerbots include-path reconciliation
#
# Vendored cmangos/playerbots uses cmangos's directory-prefixed #include style
# (`Entities/Player.h`, `Groups/Group.h`, ...). Penqle's directory layout
# differs (`Objects/Player.h`, `Group/Group.h`, ...). This script rewrites
# the qualified #include lines in src/modules/PlayerBots/ to match Penqle's
# layout.
#
# The mapping was built empirically: for each prefixed include in the
# vendored module, we located the matching basename in Penqle's src/ tree
# and recorded the actual path. 58 rewrites total, 18 exact matches (no
# rewrite needed), 13 missing-from-Penqle entries (handled separately —
# most are TBC/WotLK content gated by MANGOSBOT_ONE/TWO ifdefs at use site).
#
# Idempotent: running twice yields the same result. Re-run after re-vendoring
# cmangos/playerbots upstream.
#
# Usage: perl tools/port/rewrite-includes.pl [--dry-run]
#
# See bot-deployment-sprint-plan.md Phase 2c for context.

use strict;
use warnings;
use File::Find;
use File::Basename;

my $dry_run = grep { $_ eq '--dry-run' } @ARGV;
my $module_root = 'src/modules/PlayerBots';

# cmangos prefix → Penqle relative path under src/{game,shared,framework,scripts}/
# Order: longest-prefix-first, so MotionGenerators/PathFinder.h → Maps/PathFinder.h
# wins over MotionGenerators/* → Movement/*.
my @rules = (
    # Specific overrides first (longest match wins via @rules ordering)
    ['MotionGenerators/PathFinder.h',         'Maps/PathFinder.h'],
    ['MotionGenerators/MoveMap.h',            'Maps/MoveMap.h'],
    ['MotionGenerators/MoveMapSharedDefines.h','Maps/MoveMapSharedDefines.h'],
    ['Movement/MoveSplineInitArgs.h',         'Movement/spline/MoveSplineInitArgs.h'],
    ['Server/Opcodes.h',                      'Protocol/Opcodes.h'],
    ['Server/WorldSocket.h',                  'Protocol/WorldSocket.h'],
    ['AI/BaseAI/CreatureAI.h',                'AI/CreatureAI.h'],

    # Per-prefix dir renames
    ['Entities/GameObject.h',                 'Objects/GameObject.h'],
    ['Entities/Item.h',                       'Objects/Item.h'],
    ['Entities/ItemPrototype.h',              'Objects/ItemPrototype.h'],
    ['Entities/Object.h',                     'Objects/Object.h'],
    ['Entities/Pet.h',                        'Objects/Pet.h'],
    ['Entities/Player.h',                     'Objects/Player.h'],
    ['Entities/Transports.h',                 'Transports/Transport.h'],   # cmangos plural -> Penqle singular
    ['Entities/Unit.h',                       'Objects/Unit.h'],
    ['Object/GameObject.h',                   'Objects/GameObject.h'],
    ['Object/Player.h',                       'Objects/Player.h'],

    # File moves to root of src/game (no prefix in Penqle)
    ['Entities/GossipDef.h',                  'GossipDef.h'],
    ['Entities/ItemEnchantmentMgr.h',         'ItemEnchantmentMgr.h'],
    ['Entities/ObjectGuid.h',                 'ObjectGuid.h'],
    ['Accounts/AccountMgr.h',                 'AccountMgr.h'],
    ['DBScripts/ScriptMgr.h',                 'ScriptMgr.h'],
    ['GameEvents/GameEventMgr.h',             'GameEventMgr.h'],
    ['Globals/ObjectAccessor.h',              'ObjectAccessor.h'],
    ['Globals/ObjectMgr.h',                   'ObjectMgr.h'],
    ['Object/ObjectMgr.h',                    'ObjectMgr.h'],
    ['Globals/SharedDefines.h',               'SharedDefines.h'],
    ['Log/Log.h',                             'Log.h'],
    ['Loot/LootMgr.h',                        'LootMgr.h'],
    ['Quests/QuestDef.h',                     'QuestDef.h'],
    ['Server/WorldPacket.h',                  'WorldPacket.h'],
    ['Server/WorldSession.h',                 'WorldSession.h'],
    ['Social/SocialMgr.h',                    'SocialMgr.h'],
    ['Tools/Formulas.h',                      'Formulas.h'],
    ['Tools/Language.h',                      'Language.h'],
    ['Util/Timer.h',                          'Timer.h'],
    ['Util/Util.h',                           'Util.h'],
    ['World/World.h',                         'World.h'],

    # Dir renames
    ['BattleGround/BattleGround.h',           'Battlegrounds/BattleGround.h'],
    ['BattleGround/BattleGroundAB.h',         'Battlegrounds/BattleGroundAB.h'],
    ['BattleGround/BattleGroundAV.h',         'Battlegrounds/BattleGroundAV.h'],
    ['BattleGround/BattleGroundMgr.h',        'Battlegrounds/BattleGroundMgr.h'],
    ['BattleGround/BattleGroundWS.h',         'Battlegrounds/BattleGroundWS.h'],
    ['Combat/ThreatManager.h',                'Threat/ThreatManager.h'],
    ['Grids/CellImpl.h',                      'Maps/CellImpl.h'],
    ['Grids/GridNotifiers.h',                 'Maps/GridNotifiers.h'],
    ['Grids/GridNotifiersImpl.h',             'Maps/GridNotifiersImpl.h'],
    ['Groups/Group.h',                        'Group/Group.h'],
    ['Guilds/Guild.h',                        'Guild/Guild.h'],
    ['Guilds/GuildMgr.h',                     'Guild/GuildMgr.h'],
    ['Mails/Mail.h',                          'Mail/Mail.h'],
    ['MotionGenerators/MotionMaster.h',       'Movement/MotionMaster.h'],
    ['MotionGenerators/MovementGenerator.h',  'Movement/MovementGenerator.h'],
    ['MotionGenerators/TargetedMovementGenerator.h', 'Movement/TargetedMovementGenerator.h'],
    ['MotionGenerators/WaypointMovementGenerator.h', 'Movement/WaypointMovementGenerator.h'],
    ['Server/DBCStores.h',                    'Database/DBCStores.h'],
    ['Server/DBCStructure.h',                 'Database/DBCStructure.h'],
    ['Server/SQLStorages.h',                  'Database/SQLStorages.h'],
    ['Vmap/VMapFactory.h',                    'vmap/VMapFactory.h'],
);

# Build a regex lookup (longest match first)
my @sorted_rules = sort { length($b->[0]) <=> length($a->[0]) } @rules;

my %stats = (files_scanned => 0, files_modified => 0, includes_rewritten => 0);
my @file_changes;  # for dry-run reporting

sub process_file {
    my ($path) = @_;
    return unless $path =~ /\.(cpp|cc|c|h|hpp)$/i;
    $stats{files_scanned}++;
    open my $fh, '<', $path or do { warn "Can't read $path: $!"; return };
    my @lines = <$fh>;
    close $fh;
    my $changes = 0;
    my @local_changes;
    for my $line (@lines) {
        next unless $line =~ /^\s*#\s*include\s+"([^"]+)"/;
        my $inc = $1;
        for my $rule (@sorted_rules) {
            my ($from, $to) = @$rule;
            if ($inc eq $from) {
                $line =~ s|"\Q$from\E"|"$to"|;
                $changes++;
                push @local_changes, "$from -> $to";
                last;
            }
        }
    }
    if ($changes > 0) {
        $stats{files_modified}++;
        $stats{includes_rewritten} += $changes;
        push @file_changes, [$path, \@local_changes] if $dry_run;
        if (!$dry_run) {
            open my $out, '>', $path or do { warn "Can't write $path: $!"; return };
            print $out @lines;
            close $out;
        }
    }
}

find({ wanted => sub { process_file($File::Find::name) }, no_chdir => 1 }, $module_root);

print "rewrite-includes.pl ", ($dry_run ? "[DRY RUN]" : "[APPLIED]"), "\n";
print "  files scanned:        $stats{files_scanned}\n";
print "  files modified:       $stats{files_modified}\n";
print "  includes rewritten:   $stats{includes_rewritten}\n";

if ($dry_run && @file_changes) {
    print "\n=== Sample (first 10 modified files) ===\n";
    for my $i (0..($#file_changes < 9 ? $#file_changes : 9)) {
        my ($path, $chs) = @{$file_changes[$i]};
        print "$path\n";
        print "  $_\n" for @$chs;
    }
}
