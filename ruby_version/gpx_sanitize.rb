require 'rubygems'
require 'nokogiri'

include Nokogiri


GPX_NS = {"gpx" => "http://www.topografix.com/GPX/1/1"}

# split track when shortest distance
# between 2 points exceeds treshold
# (assuming we've got a point every 1s,
#  17 meters/s ~ 60 km/h)
#
DIST_TRESHOLD = 17

doc = Nokogiri::XML::Document.parse(File.open(ARGV[0]))

def dump_mtx mtx
  mtx.each do |r|
    r.each do |c|
      unless c[:dist].nil?
        print "\t#{sprintf("%.2f",c[:dist])}\t"
      else
        print "\t*\t"
      end
    end
    print "\n"
  end
end

# Returns an approx distance in meters
def calculate_distance(lat1, long1, lat2, long2)
  dtor = Math::PI/180
  r = 6378.14*1000

  rlat1 = lat1 * dtor
  rlong1 = long1 * dtor
  rlat2 = lat2 * dtor
  rlong2 = long2 * dtor

  dlon = rlong1 - rlong2
  dlat = rlat1 - rlat2

  a = Math::sin(dlat/2) ** 2 + Math::cos(rlat1) * Math::cos(rlat2) * (Math::sin(dlon/2) ** 2)
  c = 2 * Math::atan2(Math::sqrt(a), Math::sqrt(1-a))
  d = r * c

  return d
end

doc.remove_namespaces!

# first version: just removing anonymous traces
#

#doc.search('//trkseg').each do |node|
#  # Detects anonymized tracks
#  if node.search('trkpt/time').empty?
#    node.children.remove
#  end
#end


# Version 2: trying to recompute a kind of "shortest path" between segments
#

doc.search('//trkseg').each do |node|
  # Detects anonymized tracks
  if node.search('trkpt/time').empty?
    nodes_mtx = []
    idx = 0
    all_nodes = node.search('trkpt')
    all_nodes.each do |pt|
      nodes_mtx[idx] = []
      all_nodes.length.times do |it|
        nodes_mtx[idx][it] = {}
      end
      nodes_mtx[idx][idx] = { :dist => 0, :coords => [ pt.attr(:lat).to_f, pt.attr(:lon).to_f ] }
      idx += 1
    end
    mtx_size = nodes_mtx.length

    puts "Matrix is #{mtx_size}x#{mtx_size}-sized"

    mtx_size.times do |it|
      break if mtx_size == it + 1
      mtx_size.times do |it2|
        # skipping useless calculation
        next if it == it2
        distance_calculated = calculate_distance(nodes_mtx[it][it][:coords][0],nodes_mtx[it][it][:coords][1],
                                                 nodes_mtx[it2][it2][:coords][0], nodes_mtx[it2][it2][:coords][1])
        nodes_mtx[it][it2] = { :dist => distance_calculated }
        nodes_mtx[it2][it] = { :dist => distance_calculated }
      end
      puts "Line ##{it} of the matrix computed." if it % 500 == 0
    end
    # TODO removes after devel.
    # dumps the matrix
    # dump_mtx nodes_mtx

    remaining_pts = []
    mtx_size.times do |l|
      remaining_pts[l] = nodes_mtx[l][l][:coords]
    end

    puts remaining_pts.inspect
    cur_idx = 0
    found = 0

    # Joining closest points to each other
    # depending on the threshold
    until remaining_pts.empty?
      c_pt = remaining_pts.shift
      # finds the closest point from c_pt
      min_dist = -1
      found = -1
      mtx_size.times do |i|
        # skip iteration if current point
        next if i == cur_idx
        # init
        if min_dist == -1
          min_dist = nodes_mtx[cur_idx][i][:dist]
          found = i
        elsif nodes_mtx[cur_idx][i][:dist] < min_dist
          min_dist = nodes_mtx[cur_idx][i][:dist]
          found = i
        end
      end
      #puts "Point ##{cur_idx} has the closest point ##{found} with dist #{min_dist}"
      cur_idx += 1
    end
    node.children.remove
  end
end

# Note: Since the time needed to actually build the matrix, I began the C
# implementation, and did not finished this one.
#


