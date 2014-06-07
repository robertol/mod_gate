module SQLite

# Row/Rowset classes to simplify running queries
class Row

  def initialize(rowset, row)
    @rowset = rowset
    @row    = row
  end

  def keys()
    return @rowset.headers
  end

  def values()
    return @rowset.data[@row]
  end

  def each
    data = @rowset.data[@row]
    0.upto(data.size()-1) do |i|
      yield @rowset.header_names[i], data[i]
    end
  end

  def hashify()
    hash = {}

    self.each do |k,v|
      hash[k] = v
    end

    return hash
  end

  def [](index)
    # If column name
    if index.class == String
      # Convert to ordinal
      index = @rowset.headers[index]
    end

    if index != nil
      return @rowset.data[@row][index]
    end    
  end
end

class Rowset

  attr_reader :headers, :header_names, :data, :rows

  def initialize(db)
    @db           = db
    @headers      = {}
    @header_names = {}
    @data         = []
    @rows         = 0
  end

  # Return row at given index
  def [](row)
    return nil if row > @rows - 1

    return Row.new(self, row)
  end

  def each()
    0.upto(@rows-1) do |i|
      yield Row.new(self, i)
    end
  end

  def select(sql)
    @data = []
    stmt = @db.prepare(sql)
    
    return false if stmt == nil

    # Reset row count
    @rows = 0
    
    # Iterate over result set
    begin      
      row     = []
      ordinal = 0

      stmt.each do |name, value|        
        if @rows == 0
          @headers[name] = ordinal
          @header_names[ordinal] = name
          ordinal += 1
        else
          row << value
        end
      end

      @data << row if @rows > 0

      @rows += 1
    end while stmt.step() == SQLITE_ROW

    # Subtract 1 for header
    @rows = @rows - 1

    return true
  end
end

end # module SQLite

class X

  def initialize()
    @db = SQLite::DB.new()
    rc = @db.open("#{$modjerk_test_dir}/files/foods.db")
    
    if rc != SQLITE_OK
      puts @db.error()
    end   
  end

  def native_run()
    query = @db.prepare('select * from foods')
    
    #puts query.keys()
    while query.step() == SQLITE_ROW
      query.each do |k,v|
        puts "  #{k}: #{v}"
      end    
    end
  end

  def row_query()
    sql = %Q{ select * from foods order by name }
    
    # Create a rowset
    rowset = SQLite::Rowset.new(@db)
    
    # Select everything from queues table
    rowset.select(sql)
    
    # Iterate and print
    rowset.each do |row|
      puts "%3i %2i %-30s" % [row[0], row[1], row[2]]
    end
  end
end

x = X.new()
#x.run()
x.row_query()
