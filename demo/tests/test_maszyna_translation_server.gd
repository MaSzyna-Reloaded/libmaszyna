extends MaszynaGutTest

## MaszynaTranslationServer merges the wrapper's own catalogues and the game's .po into the one
## translation TranslationServer uses; the game's wins, and the tree re-translates on every rebuild.

const FIXTURE_PO:String = "res://tests/fixtures/lang/pl.po"
const FIXTURE_LANGUAGE:String = "pl"

## Counts the re-translations the tree is told about
class TranslationListener extends Node:
    var changes:int = 0

    func _notification(what:int) -> void:
        if what == NOTIFICATION_TRANSLATION_CHANGED:
            changes += 1


var _previous_language:String


func before_all() -> void:
    _previous_language = MaszynaTranslationServer.language
    MaszynaTranslationServer.language = FIXTURE_LANGUAGE


func before_each() -> void:
    MaszynaTranslationServer.translation_load(FIXTURE_PO)


func after_all() -> void:
    MaszynaTranslationServer.language = _previous_language
    super()


func test_translates_a_game_msgid() -> void:
    assert_eq(TranslationServer.translate("master controller"), "nastawnik jazdy")


func test_game_translation_wins_over_the_wrappers() -> void:
    assert_eq(TranslationServer.translate("forward"), "naprzód")


func test_wrapper_strings_the_game_lacks_are_translated() -> void:
    # addons/libmaszyna/translations
    assert_eq(TranslationServer.translate("cutoff"), "odcięcie")


func test_unknown_msgid_stays_as_it_is() -> void:
    assert_eq(TranslationServer.translate("no such string"), "no such string")


func test_missing_catalogue_leaves_only_the_wrappers_strings() -> void:
    MaszynaTranslationServer.translation_load("res://tests/fixtures/lang/missing.po")
    assert_eq(TranslationServer.translate("master controller"), "master controller")
    assert_eq(TranslationServer.translate("forward"), "przód")


func test_rebuild_in_the_same_language_retranslates_the_tree() -> void:
    var listener:TranslationListener = TranslationListener.new()
    add_child_autofree(listener)
    await wait_process_frames(1)
    var changes:int = listener.changes
    MaszynaTranslationServer.translation_load(FIXTURE_PO)
    assert_eq(listener.changes, changes + 1)


func test_cab_control_caption_is_a_msgid() -> void:
    assert_eq(MmdCabControlCaptions.caption(&"mainctrl"), "master controller")
    assert_eq(MmdCabControlCaptions.caption(&"battery_sw"), "battery")


func test_label_without_caption_has_none() -> void:
    assert_eq(MmdCabControlCaptions.caption(&"no_such_control"), "")


func test_universal_part_caption_carries_its_number() -> void:
    assert_eq(MmdCabControlCaptions.caption(&"universal1"),
            "%s 1" % TranslationServer.translate("interactive part"))
    assert_eq(MmdCabControlCaptions.caption(&"universalbrake1_bt"), "train brake")
