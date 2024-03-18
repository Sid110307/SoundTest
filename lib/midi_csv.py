import music21
import pandas as pd

import sys

if __name__ == "__main__":
    input_path = sys.argv[1]
    output_path = sys.argv[1].replace("mid", "csv")

    if not input_path.endswith(".mid"):
        raise ValueError("Input file must be a .mid file")
    if not output_path.endswith(".csv"):
        raise ValueError("Output file must be a .csv file")

    f = music21.midi.MidiFile()
    f.open(input_path)
    f.read()
    f.close()

    stream = music21.midi.translate.midiFileToStream(f, quantizePost=False).flat
    df = pd.DataFrame(columns=["Frequency (Hz)", "Duration (ms)"])

    for notes in stream.recurse().notes:
        if notes.isChord:
            for pitch in notes.pitches:
                pitchNote = music21.note.Note(pitch, duration=notes.duration)
                pitchNote.volume.velocity = notes.volume.velocity
                pitchNote.offset = notes.offset
                stream.insert(pitchNote)

    for note in stream.recurse().notes:
        if note.isNote:
            df = pd.concat([df, pd.DataFrame([[note.pitch.frequency, round(note.duration.quarterLength * 1000)]],
                                             columns=["Frequency (Hz)", "Duration (ms)"])], ignore_index=True)
    df.to_csv(output_path, index=False, header=False)
