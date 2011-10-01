# coding: utf-8

require 'ennou.so'
require 'webrick/log'
require 'thread'

module Rack
  module Handler
    class Ennou

      QNAME = 'Ennou_Queue'
      
      def self.setup(options)
        @logger = options[:Logger] || ::WEBrick::Log::new
        @script = ''
        if options[:config]
          if /^run\s+([^:]+)/ =~ IO::read(options[:config])
            @script = "/#{$1.chomp.downcase}"
          end
        end
        @port = options[:Port] || '80'
        if options[:Host] == '0.0.0.0'
          @host = '+'
        else
          @host = options[:Host]
          @script = ''
        end
      end

      def self.run(app, options = {})
        setup(options)
        ::Ennou::Server.open(QNAME) do |server|
          @server = server
          if @script == ''
            server.add "http://#{@host}:#{@port}/"
          else
            server.add "http://#{@host}:#{@port}#{@script}/"
            server.script = @script
          end
          @logger.info "Ennou(#{::Ennou::VERSION}) start for http://#{@host}:#{@port}#{@script}"
          loop do
            begin
              r = server.wait(60)
              next if r.nil?
              run_thread(app, *r)
            rescue Interrupt
              break
            end
          end
          @logger.info "Ennou(#{::Ennou::VERSION}) stop service for http://#{@host}:#{@port}#{@script}"
        end
      end   

      def self.shutdown
        @server.break
        @logger.info "going to shutdown ..."
      end
      
      private
      
      def self.run_thread(app, env, io)
        Thread.start do
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
            p $! if $debug
          ensure
            body.close if body.respond_to? :close
          end
        end
      end
    end
  end    
end
