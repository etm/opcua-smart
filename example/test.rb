#!/usr/bin/ruby


tools = {}
active = false
current = nil
File.read('kelch.KMT').lines.each do |l|
  l.chop!
  next if l.empty?
  if l == "#ST"
    active = true
    next
  end
  active = false if l == "#EN"
  if active
    parts = l.split(' ',2)
    if parts[0] == '#WZ'
      current = parts[1]
      tools[current] = {}
    end
  end
end
