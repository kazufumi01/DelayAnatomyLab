#pragma once
#include <juce_core/juce_core.h>
#include "Presets.h"
#include "Machines.h"

// =============================================================================
//  日本語 / English の切り替え用テキスト
//  ・プリセット解説の日本語は Presets.h、英語はここ（同じ並び順）
//  ・UI の短い文言は ui() で取得
// =============================================================================
enum class Lang { Japanese = 0, English };

struct PresetTextEN { const char* tag; const char* what; const char* use; const char* ear; };

inline const PresetTextEN& presetTextEN (int index)
{
    static const PresetTextEN texts[] =
    {
        { "All-round",
          "Quarter-note echoes return three or four times, getting slightly darker each time. Your everyday starting point for delay.",
          "Vocals, guitars, synths, anything. Put it on a send (Wet Only) and shape it from here.",
          "Feedback sets the number of repeats, Low Pass the depth, Wet the presence. Those three alone fit most songs. Track what you moved in the comparison table." },

        { "Presence",
          "A single reflection returns about 0.1 s after the source, with the top end slightly rolled off. Its origin is the 1950s tape slapback.",
          "Vocals, guitars, snare. Adds a forward, three-dimensional presence without clouding the space the way a reverb can.",
          "Toggle Bypass to hear the vocal outline thicken. Around 100 ms is the turning point: shorter feels tighter, longer feels more rockabilly." },

        { "Thickness",
          "Delays shorter than about 30 ms fuse with the source and are heard as thickness rather than a separate echo (the precedence effect). Different L/R times plus slow modulation sound like a double-tracked part.",
          "Thicker vocals, wider synths and guitars. A stand-in for a real double take.",
          "Turn on Mono check to hear the comb filtering typical of short delays. Set Depth to 0 to hear how unnatural a static double sounds." },

        { "Width",
          "The left side is the dry signal; only the right side is delayed by 12 ms. You localise toward the earlier left signal, but sound also arrives from the right, so the image spreads sideways.",
          "Spreading mono sources (guitar, pad, backing vocals) across the stereo field. Use it as an insert on the track.",
          "Always switch on Mono check. When L and R are summed, some frequencies cancel and the sound turns thin. That is the classic Haas pitfall." },

        { "Movement",
          "Echoes bounce left, right, left... Each side's feedback is routed into the opposite side (cross-feedback).",
          "Synth arpeggios, guitar riffs, vocal phrase endings. Creates movement at the sides without crowding the centre.",
          "Watch blue and red alternate on the timeline. A high-pass of 250 Hz or more keeps the echoes from fighting the bass and kick." },

        { "Groove",
          "Dotted eighth on the left, quarter note on the right. The two offset rhythms interlock and fill the gaps with sixteenth-note answers.",
          "The classic way to turn a simple eighth-note guitar or synth line into a busy rhythm. Also great behind long vocal notes.",
          "Watch the dotted-eighth echoes land off the beat lines. The lower the Low Pass, the further the echoes sit behind the source." },

        { "Texture / Build",
          "High feedback with HP/LP filters inside the loop narrows the band on every pass. Repeats get thinner, darker and wobblier as they fade.",
          "Throws on vocal endings or snare hits. Automate the send up for one beat to create transitions.",
          "Watch the colours darken on the timeline as the highs are shaved off. Around 90% feedback you are on the edge of self-oscillation (watch your level)." },

        { "Delay vs reverb",
          "Short 40-80 ms echoes with different L/R times and feedback build up densely enough to start sounding like a room.",
          "A close sense of air when a reverb pushes things too far back. Blends drums or acoustic guitar without muddying them.",
          "Sweep Feedback from 0 to 60% and find where an echo turns into ambience." },

        { "Reverse",
          "The audio is cut into slices as long as Time and played backwards. The echo starts quietly and swells to its loudest point at the end.",
          "A reversed lead-in before vocal endings, snare and cymbal swells, and the start of a new section.",
          "Toggle Reverse on and off. A longer Time means a longer run-up. Raise Feedback and the reversed slices overlap into a dreamy wash." },

        { "Pitch",
          "Each repeat rises by an octave: +12 on the first, +24 on the second... piling up into a glittering tail (Pitch = In Loop).",
          "Pads, long vocal notes, ambient textures. Tame the top with Low Pass.",
          "Set Pitch Mode to Output and every repeat stays at +12. That difference between In Loop and Output is the heart of pitch delays." },

        { "Ducking",
          "While the source is playing, Wet is lowered by up to 12 dB; when it stops the echoes bloom. The classic way to keep echoes from covering the lyrics.",
          "Vocals, rap, narration. You can keep Wet high and the words stay intelligible.",
          "Sweep Duck from 0 to 12 dB and listen to how the echoes appear between phrases. A shorter Release recovers faster; a longer one is more reserved." },

        { "Tape slapback",
          "A short Studio Tape echo returns once. Drive rounds the peaks and the top end rolls off a little, giving the repeat tape warmth behind the source.",
          "When vocals or acoustic guitar need to come forward. Unlike the Clean Slapback, the corners of the echo are rounded.",
          "Toggle against the Clean Slapback and hear what Drive and Age do to the highs and the attack." },

        { "Vocal staple",
          "Eighth-note echoes return three or four times. The tape's high-frequency loss stacks with every repeat, melting naturally behind the voice.",
          "On a vocal send, normally with Wet Only.",
          "Feedback sets the number of repeats and Age how darkly they melt. Adding Duck keeps the lyrics clear (FULL view)." },

        { "Driven throw",
          "A quarter-note echo with the tape driven hard (Drive 70%). Saturation stacks with every repeat, so the echoes get thinner and dirtier as they fade.",
          "Drum hits and guitar stabs; great as a one-beat throw on a send.",
          "The further the repeat, the more distortion and darkness. Compared with the Clean Dub Throw, these echoes distort as they go." },

        { "Rockabilly",
          "A short echo near 140 ms with preamp-style Drive and slight wobble: the thick, gently swimming slap of 1950s-60s rock.",
          "Guitar solos and vocal slap.",
          "Stronger Drive and deeper Flutter than Tape Slap, so the pitch trembles slightly." },

        { "Wobble",
          "Deep Wow and Flutter. Each repeat's pitch wavers up and down, like worn tape.",
          "Dub, psychedelia, guitar ambience.",
          "Move Wow (slow drift) and Flutter (fast tremble) separately and learn to hear the difference." },

        { "Sound-on-sound",
          "Feedback 88%: the echoes barely decay and almost self-oscillate. The highs shrink with every pass, building a thick wall like layered recording.",
          "Loop-like layering of guitar phrases and sound effects.",
          "Mind the level. The HP/LP filters inside the loop keep the self-oscillation under control." },

        { "Dub",
          "Three heads (1:2:3) place three taps in every repeat, recycled with Feedback 62%: the bouncing, stacking echo of classic dub.",
          "Snare, scat vocals, guitar chops.",
          "Check the three thin bars (taps) per repeat on the TIMELINE." },

        { "Rhythm",
          "Dotted eighth on the left, quarter on the right, with three heads. The taps draw different rhythms on each side and scatter finely.",
          "Simple chords and arpeggios.",
          "Compared with the Clean Dotted 8th+1/4, the taps add extra notes." },

        { "Ambient",
          "At 480 ms and Feedback 70%, three taps pile up and melt into the space while swaying slowly.",
          "Pads and long vocal notes.",
          "Mod Depth and Wow work together so the repeats drift." },

        { "Chorus-like",
          "BBD's short Time (45 ms) with deep Mod (2.5 ms). The delayed signal sways and widens like a chorus.",
          "Synths, clean guitar, electric piano.",
          "Set Mod Depth to 0 and the chorus feel vanishes, leaving a plain short echo." },

        { "Classic pedal",
          "A dark, sweet echo at 350 ms and Feedback 62%. It sways and rounds off with each repeat: the textbook BBD-pedal setting.",
          "Guitar solos and leads.",
          "The highs disappear as the repeats go on, leaving a warm afterglow." },

        { "Dark dub",
          "Quarter note, Feedback 70%, LP 3000 Hz. BBD darkness plus Age give a low, muffled dub with no highs.",
          "Snare throws and vocal tails.",
          "Compared with the Clean Dub Throw, the outlines vanish and the echoes fade like fog." },

        { "Smear",
          "With Oil Can's Age at 75% the repeats smear through all-pass filters and the echo melts into reverb.",
          "Pads and vocal space.",
          "Lower Age and the echoes regain their edges. Age doubles as the Diffusion control." },

        { "Slap",
          "A short 90 ms Oil Can: a soft slap with just a touch of smear.",
          "Vocals and snare.",
          "Softer-edged than a tape slap, slightly watery." },

        { "Clean",
          "Early Digital at Age 0% (16 bit, full rate): a clean digital echo with no degradation.",
          "Tight 80s-style echoes and exact rhythmic repeats.",
          "Compared with the analog machines, the repeats are the source untouched apart from decay." },

        { "Lo-Fi",
          "Age 100% (8 bit, quarter sample rate). Coarse, gritty echoes return: degradation turns coarse instead of dark.",
          "Lo-fi beats and game-style sounds.",
          "Lower Age from here and hear the quantisation noise fade." },

        { "80s",
          "Early Digital ping-pong (1/8). Age 40% adds a little grit for that 80s synth-pop texture.",
          "Synths, pads, guitars.",
          "Grainier than the Clean Ping-Pong, with the cool digital feel of the 80s." },

        { "Rock staple",
          "Dotted eighth, Feedback 30%, LP 6 kHz. Makes a guitar sound like it plays twice as many notes: the rock staple.",
          "Guitar arpeggios and riffs.",
          "It stays locked to the dotted eighth when you change tempo." },

        { "Vocal throw",
          "Quarter note, Feedback 40%, Duck 10 dB. Subtle while singing, blooming on the phrase ends.",
          "Vocals and narration.",
          "Set Duck to 0 and the echoes cover the lyrics." },

        { "Reverse lead-in",
          "Reverse at 600 ms with Feedback 0%. A reversed swell rises before the vocal starts.",
          "Vocal entrances and pre-choruses.",
          "Time is the reverse length itself. Think of it as prepared one bar ahead." },

        { "Shimmer",
          "Slightly offset L/R times (1/4T and 1/4) with +12 st In Loop. Shimmer spreads to both sides.",
          "Ambient pads and electric piano.",
          "With In Loop, every repeat climbs another octave." },

        { "Octave down",
          "Each repeat drops another octave, sinking into a heavy dub.",
          "Bassy synths and drums.",
          "At -12, -24... the repeats vanish within a few passes (watch the low end)." },

        { "Pitch doubler",
          "A short 28 ms delay detuned by +14 cents. Builds a double-tracked width.",
          "Vocals and synths.",
          "With Fine at 0 it is just a short delay; detune it and it thickens like a chorus." },
        { "Compare 1/7",
          "300 ms, Feedback 40%, LP wide open. A clean, undegraded reference. All seven A/B presets share Time, Feedback, HP/LP and Wet; only the machine differs.",
          "A yardstick: switch to the other machines and hear what each one adds.",
          "The repeats keep the same tone and fade evenly. Now step through A/B 2-7 and listen for darkness, distortion, sway, smear and grit." },
        { "Compare 2/7",
          "Same settings on Studio Tape. Drive rounds the peaks and Age rolls the highs off a little. Each repeat gets slightly darker.",
          "The reference for echoes that sit naturally behind a vocal. Subtle and refined compared with A/B 1.",
          "Switch against A/B 1 and listen for rounded attacks and repeats melting backward. Wow/Flutter are modest, so the difference is roundness rather than sway." },
        { "Compare 3/7",
          "Same settings on Echoplex: a darker hiss-like tone, a driven preamp (Drive) and Wow/Flutter sway all at once.",
          "Pick it when you want an obviously old tape-echo character: guitars, dub, gritty textures.",
          "Clearly darker, fatter and swayier than A/B 2. Distortion stacks per repeat, so later repeats get rougher." },
        { "Compare 4/7",
          "Same settings on Space Echo. Several tape heads (taps) add small reflections between repeats, so the same 300 ms feels denser.",
          "The texture where a repeat becomes a small cluster instead of a single echo. Pads, arpeggios and guitars.",
          "About as dark as A/B 3 but heard as one mass because of the extra taps. Compared with A/B 1, the rhythmic outline gets blurrier." },
        { "Compare 5/7",
          "Same settings on a BBD. The longer the delay, the slower the clock and the fewer the highs. At 300 ms it is clearly darker and sweeter than A/B 2-4.",
          "The reference for dark, sweet analog. Place synths, guitars and electric piano behind the mix.",
          "Even with LP wide open, the BBD itself removes the highs. Shorter Time gets brighter, longer gets darker (Time and tone are linked)." },
        { "Compare 6/7",
          "Same settings on an Oil Can. Electrostatic-drum diffusion makes the repeats lose their outline and melt like a reverb.",
          "Between echo and reverb. For reverb-like echoes and a sci-fi floating feel.",
          "The crisp single repeats of A/B 1 turn into a blurred mass. Raise Feedback and it stops being echo and becomes a sense of space." },
        { "Compare 7/7",
          "Same settings on Early Digital. Age 50% is about 12 bit at roughly half the sample rate. It does not darken; it roughens.",
          "80s synth-pop and lo-fi. The example of degradation that is coarse rather than dark.",
          "Contrast with A/B 5 (darkness): here the highs stay but quantisation noise adds grain, and it stacks with every repeat." },
        { "Behind the vocal",
          "Clean at a quarter note, Feedback 25%, HP 300 Hz, LP 5 kHz, Wet -14 dB, plus Duck 6 dB so the echoes step back while the voice is singing.",
          "The standard setup for opening space behind a vocal without covering the lyrics. Tempo-synced, so it blends with the song.",
          "Compare with Bypass: the vocal outline should not change, and the echoes should bloom in the phrase gaps. Set Duck to 0 and they cover the words." },
        { "Guitar depth",
          "Space Echo, 450 ms, Feedback 45%. Multi-head reflections build a small cluster behind each guitar phrase.",
          "Clean guitar, arpeggios, Rhodes. When a single echo drifts too far from the note, this adds dense depth.",
          "Switch Machine to Clean and the repeats fall back to single hits, leaving gaps between phrases. Space Echo fills the space." },
        { "Dub throw",
          "Echoplex, dotted quarter, Feedback 70%, HP 450 Hz, LP 2500 Hz. Dark, distorted repeats pile up.",
          "Dub, throws on snare or vocal. Put it on a send and ride Wet by hand.",
          "Each repeat gets darker and rougher. Move Time and the pitch slides only while it moves, so the tape feels like it is spinning." },
        { "Dark synth",
          "BBD, L 180 ms / R 270 ms, Mod Depth 2.5 ms. Because longer delays are darker on a BBD, the two sides differ slightly in tone. Chorus-like sway is added.",
          "Synth leads, pads, electric piano. When you want it behind the mix but still present.",
          "R is longer, so R is darker. Set Mod Depth to 0 and the sway disappears, leaving a plain dark echo." },
        { "Lo-fi beat",
          "Early Digital, Ping-Pong 1/8, Age 70%. Coarse echoes with reduced bit depth and sample rate bounce left and right.",
          "Lo-fi beats, game-style sounds, drum accents.",
          "It does not darken; it roughens. Lower Age and the grain returns. Listen for the darkness-versus-grit difference against the BBD." },
        { "Melt the echo",
          "Oil Can, 200 ms, Feedback 65%, Age 70%. Diffusion stacks, the echo outline disappears and it becomes ambience.",
          "When you want something between echo and reverb: add reverb-like decay to pads, guitars and drums as an extension of the echo.",
          "Switch to Clean and the repeats snap back to crisp single hits. On the Oil Can they melt and you cannot tell where they end." },
        { "Chorus",
          "Time 20 ms is swayed at 0.6 Hz and +-2.5 ms and mixed with the source. As the delay changes, the pitch drifts up and down by about +-16 cents, which creates the thickness and swirl of two voices playing together. Feedback is 0%.",
          "Guitars, electric piano, pads, synths. When you want a thickness that moves more than doubling. Keep Wet Only off and blend with the dry signal.",
          "Switch Wet Only on to hear just the pitch wobble; only when it blends with the dry signal does it become a chorus. Set Mod Depth to 0 and it turns back into plain doubling." },
        { "Chorus (stereo)",
          "Stereo with L 18 ms / R 24 ms. The sway runs in opposite phase on L and R (when L gets later, R gets earlier), so the pitch moves in opposite directions on each side and the image is wider than the mono chorus.",
          "Widening mono guitars and synths. Unlike Haas Wide, the comb-filter notches keep moving, so no single band stays thinned out.",
          "Switch Mono check on: L and R are summed and the swirl changes character, the price of the width (a mono-compatibility check). Even with Time R set to 18 ms the opposite-phase sway keeps the width." },
        { "Flanger",
          "A very short 3 ms delay is swayed at 0.25 Hz and +-2 ms, with Feedback 65% returning it. Mixed with the source it forms a comb filter (notches about 330 Hz apart: 1000 / 3 ms), and the notches sweep up and down like a jet.",
          "Guitars, hi-hats and drum loops, synths. A classic way to add motion to part of the mix.",
          "Set Feedback to 0 and the character moves toward chorus. From 65% to 85% the peaks and notches sharpen and turn metallic. If Mod Depth exceeds Time, the delay hits its lower limit and the sound gets squashed." },
        { "Flanger (stereo)",
          "Stereo with L 2.6 ms / R 3.4 ms. The L and R sweeps run in opposite phase, so the notches move in opposite directions and the sound seems to circle from side to side.",
          "Stereo drum loops, synths, and effects around a break.",
          "Sum L and R with Mono check and the oppositely moving notches mix, changing how it sounds (a mono-compatibility check). Mod Rate 0.2 -> 0.1 Hz makes it circle slowly." },
        { "Old-school chorus",
          "BBD, Stereo, Time 20 ms, a triangle LFO at 0.7 Hz and +-3 ms. A triangle rises and falls at a constant speed, so the pitch swings back and forth by a constant amount (about +-17 cents). Dark highs, noise and light distortion from the BBD add up to the feel of a 70s-80s chorus pedal.",
          "Clean guitars, electric piano, synth strings. Darker and warmer than a Clean chorus.",
          "Change LFO Shape to Sine and the turnarounds smooth out for a more modern sound. Switch the machine to Clean to hear what the BBD darkness, noise and drive were adding. Age 30 -> 70% for an even older feel." },
    };
    return texts[juce::jlimit (0, (int) (sizeof (texts) / sizeof (texts[0])) - 1, index)];
}

// 「このプリセットで触ると面白いパラメーター」（プリセットと同じ並び順）
struct TweakText { const char* ja; const char* en; };
inline const TweakText& presetTweak (int index)
{
    static const TweakText texts[] =
    {
        { "Feedback 25→60%：回数が増えて余韻になる。Low Pass 6000→2500 Hz：リピートが暗く奥へ。Wet −12→−6 dB：存在感が上がる。まずこの3つを順に動かしてみる。",
          "Feedback 25 -> 60%: more repeats, a longer tail. Low Pass 6000 -> 2500 Hz: darker, further back. Wet -12 -> -6 dB: more presence. Move these three one at a time." },
        { "Time 110→60 ms：タイトに（ダブリング寄り）。→180 ms：ロカビリー寄り。Feedback 5→35%：スラップがショート・エコーに変わる。",
          "Time 110 -> 60 ms: tighter, closer to doubling. -> 180 ms: more rockabilly. Feedback 5 -> 35%: the slap turns into a short echo." },
        { "Mod Depth 1.2→0 ms：揺れが止まり不自然になる。→3 ms：コーラスのように揺れる。Time L/R 13/21→8/34 ms：広がりが変わる。Mono を ON でコムフィルターを確認。",
          "Mod Depth 1.2 -> 0 ms: the sway stops and it sounds artificial. -> 3 ms: chorus-like. Time L/R 13/21 -> 8/34 ms: the width changes. Switch Mono on to check the comb filter." },
        { "Time R 12→5 ms：広がりが減る。→30 ms：別のエコーとして聞こえ始める。Mono Check ON で Time を動かすと、打ち消し合う周波数が移動する。",
          "Time R 12 -> 5 ms: less width. -> 30 ms: it starts to sound like a separate echo. With Mono Check on, moving Time shifts the cancelling frequencies." },
        { "Sync を 1/8→1/4：左右の間隔が倍になる。Feedback 45→70%：往復が増える。High Pass 250→600 Hz：キックやベースと干渉しなくなる。",
          "Sync 1/8 -> 1/4: the left-right spacing doubles. Feedback 45 -> 70%: more bounces. High Pass 250 -> 600 Hz: the echoes stop fighting the kick and bass." },
        { "Sync L の 1/8D を 1/8 や 1/4T に替える：リズムの絡みが変わる。Feedback 35→55%：16分の隙間が埋まる。LP 3500→1500 Hz：エコーが後ろに下がる。",
          "Change Sync L from 1/8D to 1/8 or 1/4T: the rhythmic interplay changes. Feedback 35 -> 55%: the sixteenth gaps fill in. LP 3500 -> 1500 Hz: the echoes sit further back." },
        { "Feedback 72→88%：発振寸前（音量注意）。HP 450→900 Hz／LP 2200→1200 Hz：リピートが細く暗くなる。Wet を手で上げ下げして『投げ』を作る。",
          "Feedback 72 -> 88%: on the edge of runaway (mind the level). HP 450 -> 900 Hz / LP 2200 -> 1200 Hz: thinner, darker repeats. Ride Wet by hand to make the throw." },
        { "Feedback 48→0→65%：エコーが部屋の響きに変わる境目を探す。Time L/R 43/67→20/31 ms：小さな部屋に。Mod Depth 0→1.5 ms：響きが生き物のように揺れる。",
          "Feedback 48 -> 0 -> 65%: find where echo turns into room sound. Time L/R 43/67 -> 20/31 ms: a smaller room. Mod Depth 0 -> 1.5 ms: the ambience sways like a living thing." },
        { "Time 400→800 ms：逆再生の助走が長くなる。Feedback 30→60%：スウェルが重なる。Reverse を OFF にして通常のエコーと聴き比べる。",
          "Time 400 -> 800 ms: a longer run-up. Feedback 30 -> 60%: the swells overlap. Switch Reverse off and compare with a normal echo." },
        { "Pitch 12→7 st：5度上で重なる。→−12 st：オクターブ下に沈む。Pitch Mode を Output に：全リピートが同じ高さになる。Feedback 55→75%：尾が長くきらめく。",
          "Pitch 12 -> 7 st: stacks a fifth up. -> -12 st: sinks an octave. Set Pitch Mode to Output: every repeat stays at the same pitch. Feedback 55 -> 75%: a longer, glittering tail." },
        { "Duck 12→24 dB：歌っている間はほぼ消える。Release 250→700 ms：戻りがゆっくりになる。Wet −6→0 dB：Duck があるので大きくしても言葉が聞こえる。",
          "Duck 12 -> 24 dB: almost silent while singing. Release 250 -> 700 ms: a slower recovery. Wet -6 -> 0 dB: thanks to Duck you can go louder and the words stay clear." },
        { "Drive 35→80%：リピートのアタックが丸くなり、太くなる。Age 25→70%：高域が削れて奥に引っ込む。Time 110→60 ms：スラップがダブリングに近づく（30ms以下で融合）。",
          "Drive 35 -> 80%: the repeat's attack softens and thickens. Age 25 -> 70%: the highs fall away and the echo sits back. Time 110 -> 60 ms: the slap approaches doubling (below ~30 ms it fuses)." },
        { "Age 30→60%：リピートが暗くなり歌と分離する。Feedback 30→50%：回数が増えて余韻になる。Wow 12→35%：音程が揺れて古いテープ感が出る。Duck を 6 dB 入れると歌の間だけエコーが出る。",
          "Age 30 -> 60%: darker repeats that separate from the voice. Feedback 30 -> 50%: more repeats, a longer tail. Wow 12 -> 35%: the pitch sways like old tape. Add 6 dB of Duck and the echoes appear only between phrases." },
        { "Drive 70→100%：リピートが荒く歪む。→20%：滑らかに消える。Feedback 55→75%：歪みが積み重なる。Wow 18→60%：音程がうねる。",
          "Drive 70 -> 100%: coarse, crunchy repeats; down to 20%: smooth fade. Feedback 55 -> 75%: distortion keeps stacking. Wow 18 -> 60%: the pitch swims." },
        { "Time 140→90→220 ms：ヘッド位置を動かすと、動かしている間だけピッチが滑る。Drive 55→90%：プリアンプが歪む。Flutter 12→40%：音が震える。",
          "Time 140 -> 90 -> 220 ms: the pitch slides only while the head moves. Drive 55 -> 90%: the preamp distorts. Flutter 12 -> 40%: the sound shivers." },
        { "Wow 70→0%：遅いうねりが消える。Flutter 55→0%：細かい震えが消える。片方ずつ戻すと、うねりと震えの違いが分かる。Age 60→90%：高域が丸くなる。",
          "Wow 70 -> 0%: the slow swell vanishes. Flutter 55 -> 0%: the fast shiver vanishes. Removing one at a time shows swell versus shiver. Age 60 -> 90%: the highs soften." },
        { "Feedback 88→95%：発振寸前（音量注意）。→70%：自然に減衰。Low Pass 3500→2000 Hz：塊が暗く太くなる。Wet Only で単独で聴く。",
          "Feedback 88 -> 95%: on the edge of runaway (mind the level); down to 70%: natural decay. Low Pass 3500 -> 2000 Hz: a darker, fatter mass. Use Wet Only to hear it alone." },
        { "Time 750→450 ms：タップの間隔が詰まる。Feedback 62→80%：タップが重なって濃くなる。HP 350→800 Hz：低域が抜けて軽く跳ねる。",
          "Time 750 -> 450 ms: the taps move closer. Feedback 62 -> 80%: the taps overlap and thicken. HP 350 -> 800 Hz: the lows drop out and the bounce gets lighter." },
        { "Sync L 1/8D→1/8、Sync R 1/4→1/4T：タップのずれ方が変わる。Feedback 35→55%：音符が増えて密になる。Wow 18→0%：揺れを止めてクリーンなリズムに。",
          "Sync L 1/8D -> 1/8 and Sync R 1/4 -> 1/4T: the tap offsets change. Feedback 35 -> 55%: more notes, a denser pattern. Wow 18 -> 0%: stop the sway for a clean rhythm." },
        { "Feedback 70→85%：空間が満ちていく。Mod Depth 1.0→2.5 ms：揺れが深まる。Age 55→90%：暗く溶ける。",
          "Feedback 70 -> 85%: the space fills up. Mod Depth 1.0 -> 2.5 ms: deeper sway. Age 55 -> 90%: darker melting." },
        { "Mod Depth 2.5→0→4 ms：コーラス→フラット→揺れすぎ。Time 45→120 ms：BBD は長いほど暗くなる（LPが下がる）。Rate 0.6→2 Hz：揺れが速くなりヴィブラート風。",
          "Mod Depth 2.5 -> 0 -> 4 ms: chorus, flat, too wobbly. Time 45 -> 120 ms: a BBD gets darker as Time grows (the LP drops). Rate 0.6 -> 2 Hz: faster sway, vibrato-like." },
        { "Time 350→500 ms：さらに暗くなる（Time と音色が連動）。Age 45→80%：ノイズと暗さが増す。Feedback 62→85%：発振手前のうなり。",
          "Time 350 -> 500 ms: darker still (Time and tone are linked). Age 45 -> 80%: more noise and darkness. Feedback 62 -> 85%: the growl before self-oscillation." },
        { "Age 70→100%：さらに暗くこもる。Sync 1/4→1/2：クロックが遅くなり、さらに暗くなる。Low Pass 3000→6000 Hz：ループ内LPを開けても BBD 自体が暗い。",
          "Age 70 -> 100%: darker and more muffled. Sync 1/4 -> 1/2: a slower clock, even darker. Low Pass 3000 -> 6000 Hz: even with the loop LP open, the BBD itself is dark." },
        { "Age 75→20%：粒立ちが戻る。→100%：完全に溶ける。Feedback 60→80%：にじみが積み重なりリバーブ化する。Time 220→380 ms：ドラム一周の時間が伸びる。",
          "Age 75 -> 20%: the grain returns; -> 100%: it melts completely. Feedback 60 -> 80%: the smear stacks into reverb. Time 220 -> 380 ms: the drum takes longer to come around." },
        { "Age 40→0→90%：にじみなし／強いにじみを比べる。Time 90→40 ms：Oil Can の下限。Drive 40→80%：温かく潰れる。",
          "Age 40 -> 0 -> 90%: compare no smear with heavy smear. Time 90 -> 40 ms: Oil Can's lower limit. Drive 40 -> 80%: warm squashing." },
        { "Age 0→50%：12bit・約半分のサンプルレートでザラつく。→100%：8bit の荒いエコー。Time 500→1 ms：デジタルは極短の Time も使える。",
          "Age 0 -> 50%: 12 bit at about half the sample rate, gritty. -> 100%: coarse 8 bit echoes. Time 500 -> 1 ms: digital can go extremely short." },
        { "Age 100→60→30%：ビット数が増え、徐々に滑らかになる。Feedback 45→70%：量子化ノイズが積み重なる。LP 8000→3000 Hz：粗さが丸くなる。",
          "Age 100 -> 60 -> 30%: more bits, gradually smoother. Feedback 45 -> 70%: quantisation noise stacks. LP 8000 -> 3000 Hz: the grit rounds off." },
        { "Age 40→0%：現代的でクリーンになる。Mod Depth 0.3→1.5 ms：コーラス風の揺れを足す。Feedback 40→60%：往復が増える。",
          "Age 40 -> 0%: modern and clean. Mod Depth 0.3 -> 1.5 ms: add chorus-like sway. Feedback 40 -> 60%: more bounces." },
        { "Sync 1/8D→1/8T：跳ねたリズムに。Feedback 30→50%：リピートが長くなる。Wet −9→−5 dB：エコーが前に出る。Machine を Studio Tape に替えて質感の違いを聴く。",
          "Sync 1/8D -> 1/8T: a swung rhythm. Feedback 30 -> 50%: longer repeats. Wet -9 -> -5 dB: the echoes come forward. Switch Machine to Studio Tape and hear the texture change." },
        { "Duck 10→0→20 dB：被り方を比べる。Release 300→100 ms：すぐ戻ってリピートが目立つ。Wet −4→0 dB：Duck があるので大きくできる。",
          "Duck 10 -> 0 -> 20 dB: compare how much the echoes cover the voice. Release 300 -> 100 ms: they return quickly and stand out. Wet -4 -> 0 dB: Duck lets you go louder." },
        { "Time 600→1000 ms：長いスウェルになる。Feedback 0→35%：逆再生が重なる。HP 200→500 Hz：低域を抜いて軽くする。",
          "Time 600 -> 1000 ms: a longer swell. Feedback 0 -> 35%: the reversed slices overlap. HP 200 -> 500 Hz: remove lows to lighten it." },
        { "Pitch 12→7→−12 st：5度上／オクターブ下。Pitch Mode を Output：全部同じ高さになる。Feedback 60→80%：尾が長くなる。",
          "Pitch 12 -> 7 -> -12 st: a fifth up / an octave down. Pitch Mode Output: everything stays at one pitch. Feedback 60 -> 80%: a longer tail." },
        { "Pitch −12→−5 st：緩やかに下降する。High Pass 150→300 Hz：低域のこもりを防ぐ。Feedback 55→70%：下降の回数が増える。",
          "Pitch -12 -> -5 st: a gentler descent. High Pass 150 -> 300 Hz: prevents low-end mud. Feedback 55 -> 70%: more steps down." },
        { "Fine 14→25→−14 cent：ずらす量と向き。Time 28→45 ms：先行音効果の限界（30 ms を超えるとエコーに聞こえ始める）。Mod Depth 0.4→2 ms：さらにコーラス寄りになる。",
          "Fine 14 -> 25 -> -14 cent: the amount and direction of detune. Time 28 -> 45 ms: the limit of the precedence effect (past ~30 ms it starts to sound like an echo). Mod Depth 0.4 -> 2 ms: more chorus-like." },
        { "Feedback 40→75%：リピートが長く続く（劣化がないので音色は変わらない）。LP 12000→3000 Hz：自分で暗くしてみて、A/B 2〜7 の「機種の暗さ」と比べる。",
          "Feedback 40 -> 75%: longer repeats (no degradation, so the tone stays). LP 12000 -> 3000 Hz: darken it yourself, then compare with the machine darkness in A/B 2-7." },
        { "Age 25→60%：高域が削れて奥に引っ込む。Drive 30→70%：リピートのアタックが太く丸くなる。Wow 10→30%：テープらしい音程の揺れが目立ち始める。",
          "Age 25 -> 60%: the highs fall away and the echo sits back. Drive 30 -> 70%: fatter, rounder attacks. Wow 10 -> 30%: tape-style pitch sway becomes audible." },
        { "Time 300→180→450 ms：ヘッド位置を動かすと、動かしている間だけピッチが滑る。Drive 45→80%：プリアンプが歪む。Flutter 15→40%：細かい震え。",
          "Time 300 -> 180 -> 450 ms: the pitch slides only while the head moves. Drive 45 -> 80%: the preamp distorts. Flutter 15 -> 40%: a fast shiver." },
        { "Time 300→450 ms：タップの間隔が広がり、ラテン風のリズムになる。Feedback 40→60%：タップが重なり濃くなる。HP 200→500 Hz：低域が抜けて軽く跳ねる。",
          "Time 300 -> 450 ms: wider tap spacing, a more rhythmic feel. Feedback 40 -> 60%: the taps overlap and thicken. HP 200 -> 500 Hz: lighter, bouncier." },
        { "Time 300→80→500 ms：短いと明るく、長いと暗い。Mod Depth 0→2.5 ms：コーラス風の揺れ（BBD の得意技）。Age 40→80%：ノイズと暗さが増す。",
          "Time 300 -> 80 -> 500 ms: bright when short, dark when long. Mod Depth 0 -> 2.5 ms: chorus-like sway (what BBDs do best). Age 40 -> 80%: more noise and darkness." },
        { "Age 50→0→90%：にじみなし／強いにじみを比べる。Feedback 40→70%：にじみが積み重なりリバーブ化する。Time 300→120 ms：Oil Can は短めが得意。",
          "Age 50 -> 0 -> 90%: compare no smear with heavy smear. Feedback 40 -> 70%: the smear stacks into reverb. Time 300 -> 120 ms: an Oil Can prefers shorter times." },
        { "Age 50→0→100%：クリーン／12bit／8bit を行き来する。Time 300→1 ms：デジタルは極短の Time も使える。Feedback 40→70%：粗さが積み重なる。",
          "Age 50 -> 0 -> 100%: clean / 12 bit / 8 bit. Time 300 -> 1 ms: digital can go extremely short. Feedback 40 -> 70%: the grit stacks." },
        { "Wet −14→−8 dB：存在感を上げる。Duck 6→12 dB：言葉がよりはっきり。HP 300→600 Hz：エコーが低域に被らなくなる。Sync 1/4→1/8D：リズミックに。",
          "Wet -14 -> -8 dB: more presence. Duck 6 -> 12 dB: clearer words. HP 300 -> 600 Hz: the echoes stop crowding the low end. Sync 1/4 -> 1/8D: more rhythmic." },
        { "Time 450→300→600 ms：タップの粗密が変わる。Wow 25→0%：揺れを止めて音程を安定させる。Feedback 45→65%：群れが濃くなる。",
          "Time 450 -> 300 -> 600 ms: tap density changes. Wow 25 -> 0%: stop the sway for stable pitch. Feedback 45 -> 65%: a denser cluster." },
        { "Feedback 70→85%：発振の手前（音量注意）。HP 450→900 Hz／LP 2500→1200 Hz：細く暗くなる。Time を手で動かしてピッチの滑りを聴く。",
          "Feedback 70 -> 85%: close to runaway (mind the level). HP 450 -> 900 Hz / LP 2500 -> 1200 Hz: thinner, darker. Move Time by hand and listen to the pitch slide." },
        { "Time R 270→120 ms：左右の暗さが近づく。Mod Depth 2.5→0→4 ms：コーラス→フラット→揺れすぎ。Rate 0.6→2 Hz：ヴィブラート風に。",
          "Time R 270 -> 120 ms: the two sides get closer in tone. Mod Depth 2.5 -> 0 -> 4 ms: chorus, flat, too wobbly. Rate 0.6 -> 2 Hz: vibrato-like." },
        { "Age 70→30→100%：粗さを調整。Sync 1/8→1/8D：ずれたリズムに。LP 5000→2500 Hz：粗さを丸くして馴染ませる。",
          "Age 70 -> 30 -> 100%: adjust the grit. Sync 1/8 -> 1/8D: an offset rhythm. LP 5000 -> 2500 Hz: round the grit off so it blends." },
        { "Age 70→30→100%：にじみの量を比べる。Feedback 65→80%：エコーがさらにリバーブ化する。Time 200→100 ms：Oil Can は短めが得意。",
          "Age 70 -> 30 -> 100%: compare the amount of smear. Feedback 65 -> 80%: the echo turns even more into reverb. Time 200 -> 100 ms: an Oil Can prefers shorter times." },
        { "Mod Depth 2.5→1 ms：控えめなダブリング寄り。→4 ms：揺れが目立つ。Mod Rate 0.6→2 Hz：ヴィブラート風。Time 20→10 ms：フランジャー寄りの金属感。",
          "Mod Depth 2.5 -> 1 ms: subtle, closer to doubling. -> 4 ms: obvious sway. Mod Rate 0.6 -> 2 Hz: vibrato-like. Time 20 -> 10 ms: a more metallic, flanger-like sound." },
        { "Time R 24→18 ms：左右の差が減る（それでも逆位相で広がる）。Mod Depth 3→5 ms：うねりが大きくなる。Mono Check を ON：モノ互換を確認。",
          "Time R 24 -> 18 ms: less L/R difference (the opposite-phase sway still widens it). Mod Depth 3 -> 5 ms: bigger swirl. Mono Check on: check mono compatibility." },
        { "Feedback 65→85 %：谷と山がくっきり。→0 %：コーラスに近づく。Time 3→1 ms：谷の間隔が約 1 kHz に広がる。Mod Depth 2→0 ms：スイープが止まる。",
          "Feedback 65 -> 85%: sharper peaks and notches. -> 0%: closer to chorus. Time 3 -> 1 ms: the notch spacing widens to about 1 kHz. Mod Depth 2 -> 0 ms: the sweep stops." },
        { "Time L/R 2.6/3.4→3/3 ms：左右の差が消えて、逆位相のスイープだけになる。Mod Rate 0.2→0.1 Hz：ゆっくり回る。Feedback 60→80 %：金属的に。",
          "Time L/R 2.6/3.4 -> 3/3 ms: the L/R difference vanishes, leaving only the opposite-phase sweep. Mod Rate 0.2 -> 0.1 Hz: circles slowly. Feedback 60 -> 80%: more metallic." },
        { "LFO Shape を Sine / Triangle / Square で比べる。Age 30→70 %：ノイズと暗さが増える。Machine を Clean に：BBD の色付けを外した違い。Mod Depth 3→1.5 ms：控えめに。",
          "Compare LFO Shape: Sine / Triangle / Square. Age 30 -> 70%: more noise and darkness. Machine -> Clean: hear it without the BBD colouring. Mod Depth 3 -> 1.5 ms: subtler." },
    };
    return texts[juce::jlimit (0, (int) (sizeof (texts) / sizeof (texts[0])) - 1, index)];
}

// UI の短い文言
enum class UiText { Machine, WhatHappens, WhereToUse, WhatToListen, TryThis, UserNote, Footer, ColParam, ColPreset, ColPresetCol, ColNow, Changed, SameAsPreset, Locked };

inline juce::String ui (UiText key, Lang lang)
{
    const bool ja = (lang == Lang::Japanese);
    switch (key)
    {
        case UiText::Machine:      return ja ? jp ("■ マシン") : "> Machine";
        case UiText::WhatHappens:  return ja ? jp ("■ 何が起きている？") : "> What is happening?";
        case UiText::WhereToUse:   return ja ? jp ("■ 使いどころ")       : "> Where to use it";
        case UiText::WhatToListen: return ja ? jp ("■ 聴くポイント")     : "> What to listen for";
        case UiText::TryThis:      return ja ? jp ("■ 触ってみよう（このプリセットで面白いパラメーター）") : "> Try these parameters";
        case UiText::UserNote:     return ja ? jp ("※ユーザープリセットです。解説は元になったプリセットのものを表示しています。") : "Note: this is a user preset. The text shown is from the preset it was based on.";
        case UiText::Footer:
            return ja ? jp ("※入力が無い時は SAMPLE で Pluck / Snare+Kick / Vox を選ぶと、その音にディレイが掛かります。\n"
                            "※右の表の「現在値」をクリックすると、数字を打って直接変えられます（Time は ms を入れると SYNC が外れます）。\n"
                            "※ノブは Shift + ドラッグで微調整、数値のダブルクリックで直接入力。"
                            "SYNC 中に Time を触ると同期が外れ、その音価の ms から手動で調整できます。\n"
                            "※左上の「100%」ボタンで、画面サイズと配色（6種類）を選べます。\n"
                            "※上部のプリセット欄（< > と名前）で全プリセットとユーザープリセットを選べます。「保存」で今の設定に名前を付けて残し、"
                            "「管理」から名前変更・削除・読み込み・書き出しができます。")
                      : "Tip: with no audio input, pick Pluck / Snare+Kick / Vox under SAMPLE to hear the delay.\n"
                        "Click a value in the Now column of the table to type a number directly (typing ms into Time releases SYNC).\n"
                        "Shift + drag a knob for fine control; double-click its value to type a number. "
                        "Touching Time while synced releases SYNC so you can fine-tune from that note value in ms.\n"
                        "The 100% button at the top left sets the window size and the colour skin (6 looks).\n"
                        "Use the preset bar at the top (< > and the name) to pick any factory or user preset. Save stores the current settings under a name; "
                        "Manage lets you rename, delete, import and export.";
        case UiText::ColParam:     return ja ? jp ("パラメーター") : "Parameter";
        case UiText::ColPreset:    return ja ? jp ("初期値：")     : "Preset: ";
        case UiText::ColPresetCol: return ja ? jp ("初期値")       : "Preset";
        case UiText::ColNow:       return ja ? jp ("現在値")       : "Now";
        case UiText::Changed:      return ja ? jp ("個 変更")      : " changed";
        case UiText::SameAsPreset: return ja ? jp ("= 初期値")     : "= preset";
        case UiText::Locked:       return ja ? jp ("Wet Lock 中")  : "Wet Lock";
    }
    return {};
}

// macOS の言語設定が日本語なら日本語、それ以外は英語
inline Lang systemDefaultLang()
{
    return juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("ja") ? Lang::Japanese : Lang::English;
}
