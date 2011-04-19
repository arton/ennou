# coding: utf-8

require 'ennou.so'
require 'webrick/log'
require 'thread'

module Rack
  module Handler
    class Ennou

      QNAME = 'Ennou_Queue'
      DEFAULT_THREADS = 100

      def self.run(app, options = {})
        @logger = options[:Logger] || ::WEBrick::Log::new
        queue = Queue.new
        threads = create_worker(app, queue, DEFAULT_THREADS)
        script = ''
        if options[:config]
          if /^run\s+([^:]+)/ =~ IO::read(options[:config])
            script = $1.downcase
          end
        end
        port = options[:Port] || '80'
        host = (options[:Host] == '0.0.0.0') ? '+' : options[:Host]
        ::Ennou::Server.open(QNAME) do |server|
          @server = server
          server.add "http://#{host}:#{port}/#{script}"
          @logger.info "Ennou(#{::Ennou::VERSION}) start for http://#{host}:#{port}/#{script}"
          loop do
            begin
              r = server.wait(60)
              next if r.nil?
              queue.push(r)
            rescue Interrupt
              break
            end
          end
          1.upto(threads.size) do
            queue.push [nil, nil]
          end  
          @logger.info "Ennou(#{::Ennou::VERSION}) stop service for http://#{host}:#{port}/#{script}"
        end
      end   

      def self.shutdown
        @server.break
        @logger.info "going to shutdown ..."
      end
      
      private

      def self.create_worker(app, q, count)
        ret = []
        1.upto(count) do
          ret << Thread.start do
            loop do
              env, io = q.pop
              break unless env
              env.update({'rack.version' => Rack::VERSION,
                           'rack.input' => io.input,
                           'rack.errors' => $stderr,
                           'rack.multithread' => true,
                           'rack.multiprocess' => false,
                           'rack.run_once' => false,
                           'rack.url_scheme' => env['URL_SCHEME']
                         })
              status, headers, body = app.call(env)
              begin
                io.status = status
                io.headers = headers
                body.each do |str|
                  io.write str
                end
                io.close
              rescue
              ensure
                body.close if body.respond_to? :close
              end
            end
          end
        end
        ret
      end

    end
  end    
end
