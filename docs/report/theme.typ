#let paper = rgb("#F4F3F1")
#let ink = rgb("#242221")
#let muted = rgb("#706C68")
#let card = rgb("#E7E2DC")
#let soft = rgb("#DCD7D2")
#let accent = rgb("#C48A22")
#let tech = rgb("#40536A")
#let cover = rgb("#302C2A")

#let sans = ("Droid Sans Fallback", "Source Sans 3", "DejaVu Sans")
#let mono = ("Noto Sans Mono", "DejaVu Sans Mono", "Droid Sans Fallback")

#let project-report(body) = {
  set document(title: "Future's Gaze — 未來的視線")
  set page(
    width: 210mm,
    height: 297mm,
    margin: (x: 19mm, y: 18mm),
    fill: paper,
    numbering: "1",
  )
  set text(font: sans, size: 10.6pt, fill: ink, lang: "zh")
  set figure(supplement: [圖])
  set par(justify: true, leading: 0.68em)
  set list(indent: 1.2em, body-indent: 0.45em)
  set enum(indent: 1.2em, body-indent: 0.45em)
  show raw: set text(font: mono, size: 8.6pt)
  show link: set text(fill: tech)
  show heading.where(level: 1): it => {
    pagebreak(weak: true)
    v(8pt)
    grid(
      columns: (7pt, 1fr),
      column-gutter: 10pt,
      align: horizon,
      rect(width: 7pt, height: 24pt, fill: accent),
      text(size: 22pt, weight: "bold", fill: ink)[#it.body],
    )
    v(8pt)
  }
  show heading.where(level: 2): it => {
    v(12pt)
    text(size: 15pt, weight: "bold", fill: tech)[#it.body]
    v(3pt)
  }
  show heading.where(level: 3): it => {
    v(8pt)
    text(size: 12pt, weight: "bold", fill: ink)[#it.body]
  }
  body
}

#let rulebar() = rect(width: 100%, height: 1.2pt, fill: accent)

#let pill(body) = box(
  fill: soft,
  radius: 4pt,
  inset: (x: 7pt, y: 3pt),
  text(size: 8.7pt, fill: muted)[#body],
)

#let callout(title, body, fill: card, stroke: accent) = rect(
  width: 100%,
  fill: fill,
  stroke: (left: 3pt + stroke),
  radius: 5pt,
  inset: 12pt,
)[
  #set par(justify: false)
  #text(weight: "bold", fill: stroke)[#title]
  #v(4pt)
  #body
]

#let techbox(title, body) = callout(title, body, fill: rgb("#E3E8E8"), stroke: tech)

#let img(path, caption, width: 100%) = figure(
  image(path, width: width),
  caption: text(size: 9pt, fill: muted, style: "italic")[#caption],
)

#let image-card(path, title, caption) = rect(
  width: 100%,
  fill: card,
  radius: 5pt,
  inset: 8pt,
)[
  #image(path, width: 100%)
  #v(5pt)
  #text(weight: "bold", fill: ink)[#title]
  #linebreak()
  #text(size: 9pt, fill: muted, style: "italic")[#caption]
]

#let twocol(left, right) = grid(
  columns: (1fr, 1fr),
  column-gutter: 12pt,
  row-gutter: 12pt,
  left,
  right,
)

#let wide-table(..cells) = {
  set par(justify: false)
  table(
    columns: (0.84fr, 1.55fr, 1fr),
    inset: 6pt,
    stroke: 0.35pt + rgb("#C8C1BA"),
    align: (left, left, left),
    ..cells,
  )
}

#let codeblock(body) = rect(
  width: 100%,
  fill: rgb("#ECE7E1"),
  radius: 4pt,
  inset: 9pt,
)[#body]
